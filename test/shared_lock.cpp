// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(BOOST_SAM_STANDALONE)
#define ASIO_DISABLE_BOOST_DATE_TIME 1
#else
#define BOOST_ASIO_DISABLE_BOOST_DATE_TIME 1
#endif

#include <boost/sam/guarded.hpp>
#include <boost/sam/lock_shared.hpp>
#include <boost/sam/shared_lock.hpp>
#include <boost/sam/shared_mutex.hpp>

#include <chrono>
#include <random>
#include <vector>

#if !defined(BOOST_SAM_STANDALONE)
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/yield.hpp>
#else
#include <asio/bind_cancellation_slot.hpp>
#include <asio/compose.hpp>
#include <asio/detached.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/yield.hpp>

#endif

#include "doctest.h"


using namespace BOOST_SAM_NAMESPACE;
using namespace net;
using namespace net::experimental;


inline void run_impl(io_context &ctx) { ctx.run(); }

inline void run_impl(thread_pool &ctx) { ctx.join(); }

static std::atomic<int> concurrent = 0;

struct impl : net::coroutine
{
  int                                 id;
  shared_mutex                              &mtx;
  std::shared_ptr<net::steady_timer> tim{std::make_shared<net::steady_timer>(mtx.get_executor())};

  impl(int id, bool &active, shared_mutex &mtx) : id(id), mtx(mtx) {}

  template <typename Self>
  void operator()(Self &&self, error_code ec = {}, shared_lock lock = {})
  {
    reenter(this)
    {
      printf("Entered %d\n", id);
      yield async_lock_shared(this->mtx, std::move(self));
      concurrent++;
      CHECK(concurrent > 0);
      printf("Acquired lock %d\n", id);
      assert(tim);
      tim->expires_after(std::chrono::milliseconds{10});
      yield {
        auto p = tim.get();
        p->async_wait(net::append(std::move(self), std::move(lock)));
      };
      concurrent--;
      CHECK(concurrent >= 0);
      CHECK(!ec);
      printf("Exited %d %d\n", id, ec.value());
      self.complete(ec);
    }
  }
};

template <typename CompletionToken>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
  async_impl(shared_mutex &mtx, int i, bool &active, CompletionToken &&completion_token)
{
  return net::async_compose<CompletionToken, void(error_code)>(impl(i, active, mtx), completion_token, mtx);
}

void test_sync(shared_mutex &mtx, std::vector<int> &order)
{
  bool active = false;

  static int i = 0;
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
  async_impl(mtx, i++, active, net::detached);
}

TEST_CASE_TEMPLATE("shared_lock_guard_t" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T                ctx;
  std::vector<int> order;
  shared_mutex            mtx{ctx.get_executor()};
  test_sync(mtx, order);
  run_impl(ctx);
}

struct impl_t : net::coroutine
{
  shared_mutex &mtx;

  impl_t(shared_mutex &mtx) : mtx(mtx) {}

  template <typename Self>
  void operator()(Self &&self, error_code ec = {}, shared_lock lock = {})
  {
    reenter(this)
    {
      yield async_lock_shared(this->mtx, std::move(self));
      CHECK(!ec);
      yield async_lock_shared(this->mtx, std::move(self));
      CHECK(!ec);
      yield async_lock_shared(this->mtx, std::move(self));
      CHECK(!ec);
      yield async_lock_shared(this->mtx, std::move(self));
      CHECK(!ec);
      yield async_lock_shared(this->mtx, std::move(self));
      CHECK(!ec);
      self.complete(ec);
    }
  }
};

template <typename CompletionToken>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
  async_impl(shared_mutex &mtx, CompletionToken &&completion_token)
{
  return net::async_compose<CompletionToken, void(error_code)>(impl_t(mtx), completion_token, mtx);
}

TEST_CASE_TEMPLATE("lock_series_t" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T     ctx;
  shared_mutex mtx{ctx.get_executor()};
  bool  called = false;
  async_impl(mtx,
             [&](error_code ec)
             {
                CHECK(!ec);
                called = true;
             });
  run_impl(ctx);
  mtx.lock();
  CHECK(called);
}



TEST_CASE_TEMPLATE("lock_sync" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T     ctx;
  shared_mutex mtx{ctx.get_executor()};

  ignore_unused(lock_shared(mtx));

  error_code ec;
  ignore_unused(lock_shared(mtx, ec));
  REQUIRE(!ec);
}

TEST_CASE("async_lock_cancel" * doctest::timeout(10.))
{
  net::io_context ctx;
  shared_mutex mtx{ctx.get_executor()};
  mtx.lock();
  net::cancellation_signal sig;
  async_lock_shared(mtx, net::bind_cancellation_slot(sig.slot(), net::detached));

  net::post(ctx, [&] {sig.emit(net::cancellation_type::all); });
  ctx.run();
}


TEST_CASE("lock_sync_fail")
{
  net::io_context ctx{BOOST_SAM_CONCURRENCY_HINT_1};
  shared_mutex mtx{ctx};
  shared_lock     l1 = lock_shared(mtx);
  CHECK_NOTHROW(lock_shared(mtx));

  error_code ec;
  lock_shared(mtx, ec);
  CHECK(!ec);
  mtx.lock(ec);
  CHECK(ec == net::error::in_progress);
  CHECK_THROWS(mtx.lock());
}

TEST_CASE("lock_rebind")
{
  net::io_context ctx{BOOST_SAM_CONCURRENCY_HINT_1};
  basic_shared_mutex<typename net::io_context::executor_type> mtx{ctx};
  basic_shared_lock <typename net::io_context::executor_type> l1 = lock_shared(mtx);
  shared_lock sl{std::move(l1)};
}