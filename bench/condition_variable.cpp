// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/sam/condition_variable.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/steady_timer.hpp>
#include <asio/compose.hpp>
#include <asio/detached.hpp>

#else
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/detached.hpp>
#endif

using namespace BOOST_SAM_NAMESPACE;

using steady_timer = typename net::steady_timer::rebind_executor<net::io_context::executor_type>::other;

// imitate a barrier using a timer
struct tcondvar
{
  steady_timer tim;
  tcondvar(net::io_context::executor_type exec) : tim(exec) {}

  net::io_context::executor_type get_executor() {return tim.get_executor();}

  template<typename Predicate, typename Handler>
  auto async_wait(Predicate && p, Handler && h)
  {
    return net::async_compose<Handler, void(error_code)>(
        [this, p = std::move(p), initial = true](auto && self, error_code = {}) mutable
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

        }, h, tim.get_executor());
  }

  void notify_one()
  {
    tim.cancel_one();
  }


  void notify_all()
  {
    tim.cancel();
  }
};


template<typename CondVar>
void run_benchmark(asio::io_context::executor_type exec, std::size_t n)
{
  CondVar cv{exec};

  struct impl
  {
    CondVar & cv;
    void operator()(error_code ec, std::size_t i)
    {
      if (i != 0u)
        cv.async_wait([&i = i]{return i % 4 == 0;}, net::detached);


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
  const char * name;
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  benchmark(const char * name) : name(name)
  {
  }

  ~benchmark()
  {
    auto end = std::chrono::steady_clock::now();

    printf("Benchmark  %s: %ld us\n", name, std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
  }

  operator bool () const {return true; }
};

int main(int argc, char * argv[])
{
  {net::io_context ctx{1}; }
  const std::size_t cnt = 1'000'000;
  if (auto b = benchmark("no-mutex asio"))
  {
    asio::io_context ctx{1};
    run_benchmark<tcondvar>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("no-mutex sam"))
  {
    asio::io_context ctx{1};
    run_benchmark<basic_condition_variable<net::io_context::executor_type>>(ctx.get_executor(), cnt);
  }
  if (auto b = benchmark("mutexed  asio"))
  {
    asio::io_context ctx{-1};
    run_benchmark<tcondvar>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("mutexed  sam"))
  {
    asio::io_context ctx{-1};
    run_benchmark<basic_condition_variable<net::io_context::executor_type>>(ctx.get_executor(), cnt);
  }

  return 0;
}