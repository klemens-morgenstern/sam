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


#include <boost/sam/semaphore.hpp>
#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

#include "doctest.h"
#include <iostream>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/append.hpp>
#include <asio/as_tuple.hpp>
#include <asio/coroutine.hpp>
#include <asio/deferred.hpp>
#include <asio/detached.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/yield.hpp>

#else
#include <boost/asio/append.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/yield.hpp>

#endif

using namespace BOOST_SAM_NAMESPACE;
using namespace net;

template <typename T>
constexpr int init() {return std::is_same<T, io_context>::value ? 1 : 4; }

inline void run_impl(io_context &ctx) { ctx.run(); }

inline void run_impl(thread_pool &ctx) { ctx.join(); }

template <typename T>
struct basic_bot : net::coroutine
{
  int                       n;
  semaphore                &sem;
  std::chrono::milliseconds deadline;
  basic_bot(int n, semaphore &sem, std::chrono::milliseconds deadline) : n(n), sem(sem), deadline(deadline) {}
  std::string ident = "bot " + std::to_string(n) + " : ";

  std::shared_ptr<steady_timer>         timer = std::make_shared<steady_timer>(sem.get_executor());
  std::chrono::steady_clock::time_point then;

  void say_impl() {}

  template <typename First, typename... Args>
  void say_impl(First &&first, Args &&...args)
  {
    std::cout << first;
    say_impl(std::forward<Args>(args)...);
  }
  template <typename... Args>
  void say(Args &&...args)
  {
    std::cout << ident;
    say_impl(args...);
  };

  struct op_t
  {
    semaphore                &sem;
    template<typename Token>
    BOOST_SAM_INITFN_AUTO_RESULT_TYPE(Token, void(error_code))
    operator()(Token && tk)
    {
      return sem.async_acquire(std::forward<Token>(tk));
    }

  };

  void operator()(error_code = {}, std::size_t index = 0u)
  {
    reenter(this)
    {
      say("waiting up to ", deadline.count(), "ms\n");
      then = std::chrono::steady_clock::now();
      say("approaching semaphore\n");
      if (!sem.try_acquire())
      {
        timer->expires_after(deadline);
        say("waiting up to ", deadline.count(), "ms\n");

        yield experimental::make_parallel_group(
            op_t{sem},
            timer->async_wait(net::deferred))
            .async_wait(experimental::wait_for_one(),
                        deferred([](std::array<std::size_t, 2u> seq, error_code ec_sem, error_code ec_tim)
                                 { return net::deferred.values(ec_sem, seq[0]); }))(*this);

        if (index == 1u)
        {
          say("got bored waiting after ", deadline.count(), "ms -> Cnt: ", sem.value(), "\n");
          FAIL("semaphore timeout");
          return;
        }
        else
        {
          say("semaphore acquired after ",
              std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - then).count(),
              "ms -> Cnt: ", sem.value(), "\n");
        }
      }
      else
      {
        timer->expires_after(std::chrono::milliseconds(10));
        yield {
            auto t = timer.get();
            t->async_wait(std::move(*this));
        };
        say("semaphore acquired immediately\n");
      }

      say("work done\n");

      sem.release();
      say("passed semaphore\n");
    }
  }
};

TEST_SUITE_BEGIN("basic_semaphore_test");

TEST_CASE_TEMPLATE("value" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T         ioc{init<T>()};
  semaphore sem{ioc, 0};

  CHECK(sem.value() == 0);

  sem.release();
  CHECK(sem.value() == 1);
  sem.release();
  CHECK(sem.value() == 2);

  sem.try_acquire();
  CHECK(sem.value() == 1);

  sem.try_acquire();
  CHECK(sem.value() == 0);

  sem.async_acquire(net::detached);
  CHECK(sem.value() == -1);
  sem.async_acquire(net::detached);
  CHECK(sem.value() == -2);
}

TEST_CASE_TEMPLATE("random_sem" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T                  ioc{init<T>()};
  auto               sem = semaphore(ioc.get_executor(), 1);
  std::random_device rng;
  std::seed_seq      ss{rng(), rng(), rng(), rng(), rng()};
  auto               eng  = std::default_random_engine(ss);
  auto               dist = std::uniform_int_distribution<unsigned int>(5000, 10000);

  auto random_time = [&eng, &dist] { return std::chrono::milliseconds(dist(eng)); };
  for (int i = 0; i < 100; i++)
    post(ioc, basic_bot<T>(i, sem, random_time()));
  run_impl(ioc);
}

TEST_CASE("rebind_semaphore" * doctest::timeout(10.))
{
  io_context ctx;
  auto       res = deferred.as_default_on(semaphore{ctx.get_executor()});
  res            = semaphore::rebind_executor<io_context::executor_type>::other{ctx.get_executor()};
}

TEST_CASE("sync_acquire_st" * doctest::timeout(10.))
{
  io_context ctx{1};
  semaphore  mtx{ctx};

  mtx.acquire();
  CHECK_THROWS(mtx.acquire());

  mtx.release();
  mtx.acquire();
}

TEST_CASE("sync_acquire_mt" * doctest::timeout(10.))
{
  io_context ctx;
  semaphore  mtx{ctx};

  mtx.acquire();
  post(ctx, [&] { mtx.release(); });
  std::atomic<bool> acquired{false};
  std::thread thr{[&]
                  {
                    do
                    {
                      ctx.restart();
                      std::this_thread::sleep_for(std::chrono::milliseconds(25));
                      ctx.run();
                    }
                    while (!acquired);

                  }};

  mtx.acquire();
  acquired = true;
  thr.join();
}

TEST_CASE_TEMPLATE("cancel_acquire" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  io_context ctx{init<T>()};

  std::vector<error_code> ecs;
  auto                    res = [&](error_code ec) { ecs.push_back(ec); };

  {
    semaphore::template rebind_executor<io_context::executor_type>::other mtx{ctx, 2};
    mtx.async_acquire(res);
    mtx.async_acquire(res);
    mtx.async_acquire(res);
    mtx.async_acquire(res);
    mtx.async_acquire(res);
    mtx.async_acquire(res);
    mtx.async_acquire(res);
    ctx.run_for(std::chrono::milliseconds(10));

    mtx.release();
    semaphore mt2{std::move(mtx)};
    ctx.run_for(std::chrono::milliseconds(10));

    mt2.release();
    mtx.release(); // should do nothing
  }
  ctx.run_for(std::chrono::milliseconds(10));

  CHECK(ecs.size() == 7u);
  CHECK(!ecs.at(0));
  CHECK(!ecs.at(1));
  CHECK(!ecs.at(2));
  CHECK(!ecs.at(3));

  CHECK(3u == std::count(ecs.begin(), ecs.end(), error::operation_aborted));
}

TEST_CASE_TEMPLATE("shutdown_" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  io_context ctx{init<T>()};
  auto       smtx = std::make_shared<semaphore>(ctx, 1);

  auto l = [smtx](error_code ec) { CHECK(false); };

  smtx->async_acquire(l);
  smtx->async_acquire(l);
  smtx->async_acquire(l);
}

TEST_CASE("mt_shutdown" * doctest::timeout(10.))
{
  std::weak_ptr<semaphore> wp;
  std::thread thr;
  {
    net::thread_pool ctx{2u};
    auto smtx = std::make_shared<semaphore>(ctx);
    smtx->acquire();
    std::atomic<bool> started{false};
    thr = std::thread([smtx, &started]
                      {
                        started = true;
                        CHECK_THROWS(smtx->acquire());
                      });
    wp = smtx;
    auto l =  [smtx](error_code ec) { CHECK(false); };
    while(!started)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  thr.join();
  CHECK(wp.expired());
}

TEST_SUITE_END();