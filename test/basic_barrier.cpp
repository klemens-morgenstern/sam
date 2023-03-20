// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(BOOST_SAM_STANDALONE)
#define ASIO_DISABLE_BOOST_DATE_TIME 1
#else
#define BOOST_ASIO_DISABLE_BOOST_DATE_TIME 1
#endif


#include <boost/sam/lock_guard.hpp>
#include <boost/sam/barrier.hpp>

#include "doctest.h"

#include <chrono>
#include <random>
#include <thread>

#if !defined(BOOST_SAM_STANDALONE)
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/yield.hpp>

#else
#include <asio/bind_cancellation_slot.hpp>
#include <asio/compose.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/yield.hpp>

#endif

using namespace BOOST_SAM_NAMESPACE;
using namespace net;
using namespace net::experimental;

template <typename T>
constexpr static int init() {return std::is_same<T, io_context>::value ? 1 : 4; }

inline void run_impl(io_context &ctx) { ctx.run(); }

inline void run_impl(thread_pool &ctx) { ctx.join(); }

struct basic_barrier_main_impl
{
  std::atomic<int>   done{0};
  barrier            barrier_;
  net::steady_timer tim{barrier_.get_executor()};

  basic_barrier_main_impl(net::any_io_executor exec) : barrier_{exec, 5} {}
};

struct basic_barrier_main : net::coroutine
{

  basic_barrier_main(net::any_io_executor exec) : impl_(new basic_barrier_main_impl(exec)) {}

  std::unique_ptr<basic_barrier_main_impl> impl_;

    void operator()(error_code = {})
    {
        auto p = impl_.get();
        auto &val = impl_->done;
        reenter (this)
        {
            impl_->barrier_.async_arrive([p](error_code ec){CHECK(!ec); p->done |= 1;});
            impl_->barrier_.async_arrive([p](error_code ec){CHECK(!ec); p->done |= 2;});
            impl_->barrier_.async_arrive([p](error_code ec){CHECK(!ec); p->done |= 4;});
            impl_->barrier_.async_arrive([p](error_code ec){CHECK(!ec); p->done |= 8;});
            CHECK(p->done == 0);
            yield net::post(impl_->barrier_.get_executor(), std::move(*this));
            CHECK(p->done == 0);
            yield impl_->barrier_.async_arrive(std::move(*this));
            impl_->tim.expires_after(std::chrono::milliseconds(20));
            yield impl_->tim.async_wait(std::move(*this));
            CHECK(val == 15);
        }
    }


};

TEST_SUITE_BEGIN("basic_barrier_test");

TEST_CASE_TEMPLATE("random_barrier" * doctest::timeout(1.), T, io_context, thread_pool)
{
  T ctx{init<T>()};
  net::post(ctx, basic_barrier_main{ctx.get_executor()});
  run_impl(ctx);
}

TEST_CASE("rebind_barrier" * doctest::timeout(1.))
{
  net::io_context ctx;
  auto             res = net::deferred.as_default_on(barrier{ctx.get_executor(), 4u});
  res                  = typename barrier::rebind_executor<io_context::executor_type>::other{ctx.get_executor(), 2u};
}

TEST_CASE("sync_barrier_st" * doctest::timeout(1.))
{
  net::io_context ctx{1u};
  barrier          b{ctx.get_executor(), 4u};
  CHECK_THROWS(b.arrive());

  barrier b2{ctx.get_executor(), 1u};
  CHECK_NOTHROW(b2.arrive());
}

TEST_CASE("sync_barrier_m" * doctest::timeout(1.))
{
  net::io_context ctx;
  barrier          b{ctx.get_executor(), 2u};

  net::post(ctx, [&] { b.arrive(); });
  std::atomic<bool> arrived{false};

  std::thread thr{[&]
                  {
                    do
                    {
                      ctx.restart();
                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
                      ctx.run();
                    }
                    while(!arrived);
                  }};

  CHECK_NOTHROW(b.arrive());
  arrived = true;
  thr.join();
}

TEST_CASE_TEMPLATE("shutdown_wp" * doctest::timeout(1.), T, io_context, thread_pool)
{
  T    ctx{init<T>()};
  auto smtx = std::make_shared<barrier>(ctx, 2);
  auto l    = [smtx](error_code ec) { CHECK(false); };

  smtx->async_arrive(l);
}

TEST_CASE_TEMPLATE("shutdown_" * doctest::timeout(1.), T, io_context, thread_pool)
{
  std::weak_ptr<barrier> wp;
  {
    T    ctx{init<T>()};
    auto smtx = std::make_shared<barrier>(ctx, 2);
    wp        = smtx;
    auto l    = [smtx](error_code ec) { CHECK(false); };

    smtx->async_arrive(l);
  }

  CHECK(wp.expired());
}

TEST_CASE_TEMPLATE("cancel" * doctest::timeout(1.), T, io_context, thread_pool)
{
  T    ctx{init<T>()};
  auto smtx = std::make_shared<barrier>(ctx, 2);
  auto l    = [smtx](error_code ec) { CHECK(ec == net::error::operation_aborted); };

  net::cancellation_signal sig;

  smtx->async_arrive(net::bind_cancellation_slot(sig.slot(), l));

  net::post([&] { sig.emit(net::cancellation_type::total); });

  run_impl(ctx);
}

TEST_CASE("mt_shutdown" * doctest::timeout(1.))
{
  std::weak_ptr<barrier> wp;
  std::thread thr;
  std::exception_ptr ep;
  {
    net::thread_pool ctx{2u};
    auto smtx = std::make_shared<barrier>(ctx, 3);
    std::atomic<bool> started{false};
    thr = std::thread(
       [smtx, &started]
       {
         started = true;
         CHECK_THROWS_AS(smtx->arrive(), system_error);
       });

    wp = smtx;
    auto l =  [smtx](error_code ec) { CHECK(false); };
    while(!started)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    smtx->async_arrive(l);
  }

  thr.join();
  CHECK(wp.expired());
}

TEST_SUITE_END();