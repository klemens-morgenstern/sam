// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(BOOST_SAM_STANDALONE)
#define ASIO_DISABLE_BOOST_DATE_TIME 1
#else
#define BOOST_ASIO_DISABLE_BOOST_DATE_TIME 1
#endif

#include <boost/sam/condition_variable.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/compose.hpp>
#include <asio/detached.hpp>
#include <asio/steady_timer.hpp>
#else
#include <boost/asio/compose.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#endif

using namespace BOOST_SAM_NAMESPACE;

using steady_timer = typename net::steady_timer::rebind_executor<net::io_context::executor_type>::other;

// imitate a barrier using a timer
struct tcondvar
{
  steady_timer tim;
  tcondvar(net::io_context::executor_type exec) : tim(exec) {}

  net::io_context::executor_type get_executor() { return tim.get_executor(); }

  template<typename Predicate>
  struct async_wait_op
  {
    Predicate p;
    steady_timer &tim;

    template<typename Predicate_>
    async_wait_op(Predicate_ && p, steady_timer &tim)
      : p(std::forward<Predicate_>(p)), tim(tim) {}

    bool initial = true;

    template<typename Self>
    void operator()(Self &&self, error_code = {})
    {
      if (initial)
      {
        initial = false;
        return net::post(std::move(self));
      }
      error_code ec;
      if (self.cancelled() != net::cancellation_type::none)
        ec = net::error::operation_aborted;
      if (ec || p())
        self.complete(ec);
      else
        tim.async_wait(std::move(self));
    }
  };

  template <typename Predicate, typename Handler>
  void async_wait(Predicate &&p, Handler &&h)
  {
    return net::async_compose<Handler, void(error_code)>(
        async_wait_op<typename std::decay<Predicate>::type>(std::move(p), tim), h, tim);
  }

  void notify_one() { tim.cancel_one(); }

  void notify_all() { tim.cancel(); }
};

template <typename CondVar>
void run_benchmark(net::io_context::executor_type exec, std::size_t n)
{
  CondVar cv{exec};

  struct impl
  {
    CondVar &cv;
    void     operator()(error_code ec, std::size_t i)
    {
      if (i != 0u)
        cv.async_wait([i] { return i % 4 == 0; }, net::detached);

      if (i % 100 == 0)
        cv.notify_all();
      else
        cv.notify_one();
      if (i != 0)
        net::post(cv.get_executor(), net::append(*this, ec, i - 1));
    };
  };

  net::post(exec, net::append(impl{cv}, error_code(), n));

  exec.context().run();
}

struct benchmark
{
  const char                           *name;
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  benchmark(const char *name) : name(name) {}

  ~benchmark()
  {
    auto end = std::chrono::steady_clock::now();

    printf("Benchmark  %s: %ld us\n", name, std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
  }

  operator bool() const { return true; }
};

int main(int argc, char *argv[])
{
  {
    net::io_context ctx{1};
    ctx.run();
  }
  const std::size_t cnt = 1000000;
  if (auto b = benchmark("no-mutex asio"))
  {
    net::io_context ctx{1};
    run_benchmark<tcondvar>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("no-mutex  sam"))
  {
    net::io_context ctx{1};
    run_benchmark<basic_condition_variable<net::io_context::executor_type>>(ctx.get_executor(), cnt);
  }
  if (auto b = benchmark("mutexed  asio"))
  {
    net::io_context ctx{-1};
    run_benchmark<tcondvar>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("mutexed   sam"))
  {
    net::io_context ctx{-1};
    run_benchmark<basic_condition_variable<net::io_context::executor_type>>(ctx.get_executor(), cnt);
  }

  return 0;
}