// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/sam/mutex.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/steady_timer.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/experimental/concurrent_channel.hpp>
#include <asio/compose.hpp>
#include <asio/coroutine.hpp>
#include <asio/detached.hpp>
#include <asio/yield.hpp>

#else
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/experimental/concurrent_channel.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/yield.hpp>

#endif

using namespace BOOST_SAM_NAMESPACE;

// imitate a barrier using a timer
template<template <typename ...> class Channel>
struct tmutex
{
  Channel<void(error_code)> chan;
  tmutex(net::io_context::executor_type exec) : chan(exec, 1) {}

  template<typename Handler>
  auto async_lock(Handler && h)
  {
    return chan.async_send(error_code(), std::move(h));
  }

  bool try_lock()
  {
    return chan.try_send(error_code());
  }

  void unlock()
  {
    chan.try_receive([](error_code) {});
  }


};

template<typename Mutex>
struct run_benchmark_impl : net::coroutine
{
  std::size_t N;
  Mutex &mtx;

  void operator()(error_code ec = {})
  {
    reenter (this)
    {
      while ( 0 < N--)
      {
        if (!mtx.try_lock())
          yield { mtx.async_lock(std::move(*this)); }
        mtx.unlock();
      }
    }

  }
};

template<typename Mutex>
void run_benchmark(asio::io_context::executor_type exec, std::size_t n)
{
  Mutex cv{exec};

  net::post(exec, run_benchmark_impl<Mutex>{{}, n, cv});

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
    printf("Benchmark  %s: %ld us\n", name,
           std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
  }

  operator bool () const {return true; }
};


int main(int argc, char * argv[])
{
  {net::io_context ctx{1};}
  const std::size_t cnt = 10'000'000;
  if (auto b = benchmark("no-mutex asio"))
  {
    net::io_context ctx{1};
    run_benchmark<tmutex<net::experimental::channel>>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("no-mutex sam"))
  {
    net::io_context ctx{1};
    run_benchmark<basic_mutex<net::io_context::executor_type>>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("mutexed asio"))
  {
    net::io_context ctx{-1};
    run_benchmark<tmutex<net::experimental::concurrent_channel>>(ctx.get_executor(), cnt);
  }

  if (auto b = benchmark("mutexed sam"))
  {
    net::io_context ctx{-1};
    run_benchmark<basic_mutex<net::io_context::executor_type>>(ctx.get_executor(), cnt);
  }
  return 0;
}