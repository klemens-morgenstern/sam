// Copyright (c) 2022 Richard Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.


#if defined(BOOST_SAM_STANDALONE)
#define ASIO_DISABLE_BOOST_DATE_TIME 1
#else
#define BOOST_ASIO_DISABLE_BOOST_DATE_TIME 1
#endif

#include <boost/sam/condition_variable.hpp>
#include <boost/sam/mutex.hpp>
#include <boost/sam/unique_lock.hpp>
#include <chrono>
#include <atomic>
#include <random>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/bind_cancellation_slot.hpp>
#include <asio/deferred.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#else
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/thread_pool.hpp>
#include <thread>
#endif

#include "doctest.h"


using namespace BOOST_SAM_NAMESPACE;
using namespace net;

template <typename T>
constexpr static int init() {return std::is_same<T, io_context>::value ? 1 : 4; }

inline void run_impl(io_context &ctx) { ctx.run(); }

inline void run_impl(thread_pool &ctx) { ctx.join(); }

TEST_SUITE_BEGIN("basic_condition_variable_test");

TEST_CASE_TEMPLATE("cancel_all" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
    T ioc{init<T>()};
    auto l =
            [&]
            {
                auto cv  = condition_variable(ioc.get_executor());
                cv.async_wait([](error_code ec){CHECK(ec == net::error::operation_aborted);});
                cv.async_wait([](error_code ec){CHECK(ec == net::error::operation_aborted);});
                cv.async_wait([](error_code ec){CHECK(ec == net::error::operation_aborted);});
                cv.async_wait([](error_code ec){CHECK(ec == net::error::operation_aborted);});
            };
    post(ioc, l);
    run_impl(ioc);
}

TEST_CASE_TEMPLATE("notify_all" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T                ioc{init<T>()};
  auto             cv  = condition_variable(ioc.get_executor());
  std::atomic<int> cnt{0};
  cv.async_wait(
      [&](error_code ec)
      {
        cnt |= 1;
        CHECK(!ec);
      });
  cv.async_wait(
      [&](error_code ec)
      {
        cnt |= 2;
        CHECK(!ec);
      });
  cv.async_wait(
      [&](error_code ec)
      {
        cnt |= 4;
        CHECK(!ec);
      });
  cv.async_wait(
      [&](error_code ec)
      {
        cnt |= 8;
        CHECK(!ec);
      });
  post(ioc, [&] { cv.notify_all(); });
  run_impl(ioc);

  CHECK(cnt == 15);
}

TEST_CASE_TEMPLATE("notify_one" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
    T ioc{init<T>()};
    std::unique_ptr<condition_variable> store{new condition_variable(ioc.get_executor())};
    auto & cv = *store;
    std::atomic<int> cnt{0};
    cv.notify_one();

    cv.async_wait([&](error_code ec){if (!ec) cnt |= 1; CHECK(!ec);});
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 2; CHECK(!ec);});
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 4; CHECK(ec == net::error::operation_aborted);});
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 8; CHECK(ec == net::error::operation_aborted);});
    cv.notify_one();
    cv.notify_one();

  net::steady_timer tim{ioc, std::chrono::milliseconds{100}};
  tim.async_wait([&](error_code ec) { store.reset(); });

  run_impl(ioc);
  CHECK_GT(cnt, 2);
}

TEST_CASE_TEMPLATE("notify_some_more" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
    T ioc{init<T>()};
    std::unique_ptr<condition_variable> store{new condition_variable(ioc.get_executor())};
    auto & cv = *store;

    std::atomic<int> cnt{0};
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 1; CHECK(!ec);});
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 2; CHECK(!ec);});
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 4; CHECK(!ec);});
    cv.async_wait([&](error_code ec){if (!ec) cnt |= 8; CHECK(ec == net::error::operation_aborted); });

  cv.notify_one();
  cv.notify_one();
  cv.notify_one();

  net::steady_timer tim{ioc, std::chrono::milliseconds{100}};
  tim.async_wait([&](error_code ec) { store.reset(); });

  run_impl(ioc);
  CHECK_GT(cnt, 5);
}

TEST_CASE("rebind_condition_variable" * doctest::timeout(10.))
{
  net::io_context ctx;
  auto            res = net::deferred.as_default_on(condition_variable{ctx.get_executor()});

  res = condition_variable{ctx.get_executor()};
}

TEST_CASE_TEMPLATE("shutdown_" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  io_context ctx{init<T>()};
  auto       smtx = std::make_shared<condition_variable>(ctx);

  auto l = [smtx](error_code ec) { CHECK(false); };

  smtx->async_wait(l);
}

TEST_CASE_TEMPLATE("cancel_" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T    ctx{init<T>()};
  auto smtx = std::make_shared<condition_variable>(ctx);

  net::cancellation_signal csig;
  smtx->async_wait([&](error_code ec)
                   {
                     CHECK(!ec);
                     csig.emit(net::cancellation_type::all);
                   });
  smtx->async_wait(net::bind_cancellation_slot(csig.slot(), [&](error_code ec)
                                               { CHECK(ec == net::error::operation_aborted); }));

  net::post(ctx, [&] { smtx->notify_one(); });
  run_impl(ctx);
}

TEST_CASE_TEMPLATE("cancel_2" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T    ctx{init<T>()};
  auto smtx = std::make_shared<condition_variable>(ctx);

  net::cancellation_signal csig;
  smtx->async_wait(net::bind_cancellation_slot(csig.slot(), [&](error_code ec)
                                               { CHECK(ec == net::error::operation_aborted); }));

  net::post(ctx, [&] { csig.emit(net::cancellation_type::all); });
  run_impl(ctx);
}

TEST_CASE("wait" * doctest::timeout(10.))
{
  net::io_context ctx;
  condition_variable cv{ctx};
  std::atomic<bool> inited{false};
  std::thread thr{[&]{inited = true; cv.wait();}};

  while(!inited)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  cv.notify_one();
  thr.join();

}

TEST_SUITE_END();