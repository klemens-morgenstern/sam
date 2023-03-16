// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

#include <boost/sam/guarded.hpp>
#include <boost/sam/mutex.hpp>
#include <boost/sam/semaphore.hpp>
#include <chrono>
#include <random>
#include <vector>

#if !defined(BOOST_SAM_STANDALONE)
namespace asio = boost::asio;
#include <boost/asio.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/yield.hpp>
#else

#include <asio.hpp>
#include <asio/compose.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/yield.hpp>

#endif

using namespace BOOST_SAM_NAMESPACE;
using namespace net;
using namespace net::experimental;

using models = std::tuple<net::io_context, net::thread_pool>;

inline void run_impl(io_context &ctx) { ctx.run(); }

inline void run_impl(thread_pool &ctx) { ctx.join(); }

static int concurrent = 0;
static int cmp        = 0;

struct impl
{
  int                                 id;
  std::shared_ptr<asio::steady_timer> tim;

  impl(int id, bool &active, asio::any_io_executor exec) : id(id), tim{std::make_shared<asio::steady_timer>(exec)}
  {
    assert(exec);
  }

  template <typename Self>
  void operator()(Self &&self)
  {
    BOOST_CHECK_LE(concurrent, cmp);
    concurrent++;
    printf("Entered %d\n", id);
    tim->expires_after(std::chrono::milliseconds{10});
    tim->async_wait(std::move(self));
  }

  template <typename Self>
  void operator()(Self &&self, error_code ec)
  {
    BOOST_CHECK(!ec);
    printf("Exited %d %d\n", id, ec.value());
    concurrent--;
    self.complete(ec);
  }
};

template <typename T, typename CompletionToken>
auto async_impl(T &se, int i, bool &active, CompletionToken &&completion_token)
{
  return net::async_compose<CompletionToken, void(error_code)>(impl(i, active, se.get_executor()), completion_token,
                                                               se.get_executor());
}

template <typename T>
void test_sync(T &se2, std::vector<int> &order)
{
  bool active = false;
  auto op     = [&](auto &&token)
  {
    static int i = 0;
    fprintf(stderr, "Op  %d\n", i);
    return async_impl(se2, i++, active, std::move(token));
  };

  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
  guarded(se2, op, asio::detached);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(guarded_semaphore_test, T, models)
{
  T         ctx;
  semaphore se{ctx.get_executor(), 3};
  cmp = 3;
  std::vector<int> order;
  test_sync<semaphore>(se, order);
  run_impl(ctx);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(guarded_mutex_test, T, models)
{
  T                ctx;
  std::vector<int> order;
  mutex            mtx{ctx.get_executor()};
  cmp = 1;
  test_sync<mutex>(mtx, order);
  run_impl(ctx);
}