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
#include <boost/sam/mutex.hpp>
#include <boost/sam/semaphore.hpp>

#include <chrono>
#include <random>
#include <vector>

#if !defined(BOOST_SAM_STANDALONE)
#include <boost/asio/compose.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/thread_pool.hpp>
#else
#include <asio/compose.hpp>
#include <asio/detached.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>

#endif

#include "doctest.h"


using namespace BOOST_SAM_NAMESPACE;
using namespace net;
using namespace net::experimental;

inline void run_impl(io_context &ctx) { ctx.run(); }

inline void run_impl(thread_pool &ctx) { ctx.join(); }

static std::atomic<int> concurrent{0};
static std::atomic<int> cmp{0};
struct impl
{
  int                                 id;
  std::shared_ptr<net::steady_timer> tim;

  impl(int id, std::atomic<bool> &active, net::any_io_executor exec) : id(id), tim{std::make_shared<net::steady_timer>(exec)}
  {
    REQUIRE(exec);
  }

  template <typename Self>
  void operator()(Self &&self)
  {
    CHECK(concurrent <= cmp);
    concurrent++;
    printf("Entered %d\n", id);
    tim->expires_after(std::chrono::milliseconds{10});
    tim->async_wait(std::move(self));
  }

  template <typename Self>
  void operator()(Self &&self, error_code ec)
  {
    CHECK(!ec);
    printf("Exited %d %d\n", id, ec.value());
    concurrent--;
    self.complete(ec);
  }
};

template <typename T, typename CompletionToken>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
  async_impl(T &se, int i, std::atomic<bool> &active, CompletionToken &&completion_token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      impl(i, active, se.get_executor()), completion_token,
      se.get_executor());
}

template<typename T>
struct op_t
{
    T & se2;
    std::atomic<bool> &active;

    template<typename Token>
    auto operator()(Token && token)
        -> decltype(async_impl(std::declval<T&>(), 0, std::declval<std::atomic<bool>&>(), std::forward<Token>(token)))
    {
      static std::atomic<int> i{0};
      fprintf(stderr, "Op  %d\n", i.load());
      return async_impl(se2, i++, active, std::forward<Token>(token));
    }
};

template <typename T>
void test_sync(T &se2, std::vector<int> &order)
{
    std::atomic<bool> active{false};
    op_t<T> op{se2, active};

  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
  guarded(se2, op, net::detached);
}

TEST_CASE_TEMPLATE("guarded_semaphore_test" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T         ctx;
  semaphore se{ctx.get_executor(), 3};
  cmp = 3;
  std::vector<int> order;
  test_sync<semaphore>(se, order);
  run_impl(ctx);
}

TEST_CASE_TEMPLATE("guarded_mutex_test" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T                ctx;
  std::vector<int> order;
  mutex            mtx{ctx.get_executor()};
  cmp = 1;
  test_sync<mutex>(mtx, order);
  run_impl(ctx);
}

template<typename T>
struct once_op
{
  T & ctx;
  std::unique_ptr<int> ptr;

  template<typename Token>
  auto operator()(Token && token) &&
    -> decltype(net::post(ctx, net::append(std::forward<Token>(token), error_code(), std::move(ptr))))
  {
    REQUIRE(ptr != nullptr);
    return net::post(ctx, net::append(std::forward<Token>(token), error_code(), std::move(ptr)));
  }
};

// basically just a compile test
TEST_CASE_TEMPLATE("guarded_mutex_deferred_test" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T                    ctx;
  int *                pp;
  std::unique_ptr<int> ptr{pp = new int(42)};
  mutex                mtx{ctx.get_executor()};
  guarded(mtx,
          once_op<T>{ctx, std::move(ptr)},
          [&](error_code ec, std::unique_ptr<int> ptr_)
          {
            CHECK(ec == error_code{});
            CHECK(ptr == nullptr);
            CHECK(ptr_.get() == pp);
            CHECK(*pp == 42);
          });
  run_impl(ctx);
}


// basically just a compile test
TEST_CASE_TEMPLATE("guarded_semaphore_deferred_test" * doctest::timeout(10.), T, net::io_context, net::thread_pool)
{
  T                    ctx;
  std::unique_ptr<int> ptr{new int(42)};
  int *                pp = ptr.get();
  semaphore            sem{ctx.get_executor()};

  guarded(sem,
          net::post(ctx, net::append(net::deferred, error_code{}, std::move(ptr))),
          [&](error_code ec, std::unique_ptr<int> ptr_)
          {
            CHECK(ec == error_code{});
            CHECK(ptr == nullptr);
            CHECK(ptr_.get() == pp);
            CHECK(*pp == 42);
          });

  run_impl(ctx);
}