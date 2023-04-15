// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(BOOST_SAM_STANDALONE)
#define ASIO_DISABLE_BOOST_DATE_TIME 1
#else
#define BOOST_ASIO_DISABLE_BOOST_DATE_TIME 1
#endif

#include <algorithm>
#include <boost/sam/lock.hpp>
#include <boost/sam/mutex.hpp>
#include <boost/sam/unique_lock.hpp>
#include <chrono>
#include <random>

#include <thread>
#include "doctest.h"


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

template <typename T>
struct basic_main_impl
{
  mutex            mtx;
  std::vector<int> seq;

  basic_main_impl(net::any_io_executor exec) : mtx{exec} {}
};

template <typename T>
struct basic_main : net::coroutine
{
  struct step_impl
  {
    std::vector<int> &v;
    mutex            &mtx;
    int               i;

    std::unique_ptr<net::steady_timer> tim;

    template <typename Self>
    void operator()(Self &self)
    {
      async_lock(mtx, std::move(self));
    }

    template <typename Self>
    void operator()(Self &self, error_code ec, unique_lock l)
    {
      v.push_back(i);
      tim.reset(new net::steady_timer(mtx.get_executor(), std::chrono::milliseconds(10)));
      tim->async_wait(std::move(self));
      v.push_back(i + 1);
    };

    template <typename Self>
    void operator()(Self &self, error_code ec)
    {
      self.complete(ec);
    }
  };

  basic_main(net::any_io_executor exec) : impl_(new basic_main_impl<T>(exec)) {}

  std::unique_ptr<basic_main_impl<T>> impl_;

  static auto f(std::vector<int> &v, mutex &mtx, int i) -> net::deferred_async_operation<
      void(error_code), net::detail::initiate_composed_op<void(error_code), void(net::any_io_executor)>, step_impl>
  {
    return async_compose<const net::deferred_t &, void(error_code)>(step_impl{v, mtx, i}, net::deferred, mtx);
  }

  void operator()(error_code = {})
  {
    reenter(this)
    {
      yield make_parallel_group(f(impl_->seq, impl_->mtx, 0), f(impl_->seq, impl_->mtx, 3),
                                f(impl_->seq, impl_->mtx, 6), f(impl_->seq, impl_->mtx, 9))
          .async_wait(wait_for_all(), std::move(*this));
    }
  }

  void operator()(std::array<std::size_t, 4> order, error_code ec1, error_code ec2, error_code ec3, error_code ec4)
  {
    CHECK(!ec1);
    CHECK(!ec2);
    CHECK(!ec3);
    CHECK(!ec4);
    CHECK(impl_->seq.size() == 8);
    CHECK((impl_->seq[0] + 1) == impl_->seq[1]);
    CHECK((impl_->seq[2] + 1) == impl_->seq[3]);
    CHECK((impl_->seq[4] + 1) == impl_->seq[5]);
    CHECK((impl_->seq[6] + 1) == impl_->seq[7]);
  }
};

TEST_SUITE_BEGIN("basic_mutex_test");

TEST_CASE_TEMPLATE("random_mtx" * doctest::timeout(10.), T, io_context, thread_pool)
{
  T ctx{init<T>()};
  net::post(ctx, basic_main<T>{ctx.get_executor()});
  run_impl(ctx);
}

TEST_CASE("rebind_mutex" * doctest::timeout(10.))
{
  net::io_context ctx;
  auto            res = net::deferred.as_default_on(mutex{ctx.get_executor()});
}

TEST_CASE("sync_lock_st" * doctest::timeout(10.))
{
  net::io_context ctx{1u};
  mutex            mtx{ctx};

  mtx.lock();
  CHECK_THROWS(mtx.lock());

  mtx.unlock();
  mtx.lock();
}

TEST_CASE("sync_lock_mt" * doctest::timeout(10.))
{
  net::io_context ctx;
  mutex            mtx{ctx};

  mtx.lock();
  net::post(ctx, [&] { mtx.unlock(); });
  std::atomic<bool> locked{false};

  std::thread thr{[&]
                  {
                    do
                    {
                      ctx.restart();
                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
                      ctx.run();
                    }
                    while(!locked);
                  }};

  mtx.lock();
  locked = true;
  thr.join();
}

TEST_CASE_TEMPLATE("multi_lock" * doctest::timeout(10.), T, io_context, thread_pool)
{
  T     ctx{init<T>()};
  mutex mtx{ctx};

  run_impl(ctx);
}

TEST_CASE_TEMPLATE("cancel_twice" * doctest::timeout(10.), T, io_context, thread_pool)
{
  net::io_context        ctx{init<T>()};
  std::vector<error_code> ecs;
  auto                    res = [&](error_code ec) { ecs.push_back(ec); };

  {
    mutex mtx{ctx};
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);

    ctx.run_for(std::chrono::milliseconds(10));

    mtx.unlock();
    ctx.run_for(std::chrono::milliseconds(10));

    mtx.unlock();
    ctx.run_for(std::chrono::milliseconds(10));
  }
  ctx.run_for(std::chrono::milliseconds(10));

  CHECK(ecs.size() == 7u);
  CHECK(!ecs.at(0));
  CHECK(!ecs.at(1));
  CHECK(!ecs.at(2));

  CHECK(4u == std::count(ecs.begin(), ecs.end(), error::operation_aborted));
}

TEST_CASE_TEMPLATE("cancel_lock" * doctest::timeout(10.), T, io_context, thread_pool)
{
  net::io_context ctx{init<T>()};

  std::vector<error_code> ecs;
  auto                    res = [&](error_code ec) { ecs.push_back(ec); };

  {
    mutex::template rebind_executor<net::io_context::executor_type>::other mtx{ctx};
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    mtx.async_lock(res);
    ctx.run_for(std::chrono::milliseconds(10));

    mtx.unlock();
    mutex mt2{std::move(mtx)};
    ctx.run_for(std::chrono::milliseconds(10));

    mt2.unlock();
    mtx.unlock(); // should do nothing
  }
  ctx.run_for(std::chrono::milliseconds(10));

  CHECK(ecs.size() == 7u);
  CHECK(!ecs.at(0));
  CHECK(!ecs.at(1));
  CHECK(!ecs.at(2));

  CHECK(4u == std::count(ecs.begin(), ecs.end(), error::operation_aborted));
}

TEST_CASE_TEMPLATE("shutdown_" * doctest::timeout(10.), T, io_context, thread_pool)
{
  io_context ctx{init<T>()};
  auto       smtx = std::make_shared<mutex>(ctx);

  auto l = [smtx](error_code ec) { CHECK(false); };

  smtx->async_lock(l);
  smtx->async_lock(l);
}

TEST_CASE_TEMPLATE("cancel_" * doctest::timeout(10.), T, io_context, thread_pool)
{
  T    ctx{init<T>()};
  auto smtx = std::make_shared<mutex>(ctx);

  auto                     l = [smtx](error_code ec) { CHECK(false); };
  net::cancellation_signal csig;
  smtx->lock();
  smtx->async_lock(
      [&](error_code ec)
      {
        CHECK(!ec);
        csig.emit(net::cancellation_type::all);
      });
  smtx->async_lock(net::bind_cancellation_slot(
        csig.slot(),
        [&](error_code ec)
        {
          CHECK(ec == net::error::operation_aborted);
        }
        ));

  smtx->unlock();
  run_impl(ctx);
}

TEST_CASE("mt_shutdown" * doctest::timeout(10.))
{
  std::weak_ptr<mutex> wp;
  std::thread thr;
  {
    net::thread_pool ctx{2u};
    auto smtx = std::make_shared<mutex>(ctx);
    smtx->lock();
    std::atomic<bool> started{false};
    thr = std::thread(
        [smtx, &started]
        {
          started = true;
          CHECK_THROWS(smtx->lock());
        });
    wp = smtx;
    auto l =  [smtx](error_code ec) { CHECK(false); };
    while(!started)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  thr.join();
  CHECK(wp.expired());
}


TEST_CASE("holds-work" * doctest::timeout(10.))
{
  io_context ctx{1u};
  mutex mtx{ctx};
  mtx.lock();

  auto l = [](error_code ec) { CHECK(false); };
  mtx.async_lock(l);
  mtx.async_lock(l);

  CHECK(ctx.run_for(std::chrono::milliseconds(1000)) == 0);
}



TEST_SUITE_END();