// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sam/detail/concurrency_hint.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/thread_pool.hpp>
#else
#include <boost/asio/thread_pool.hpp>
#endif

#include "doctest.h"

TEST_SUITE_BEGIN("concurrency_hint");

using namespace BOOST_SAM_NAMESPACE;
using BOOST_SAM_NAMESPACE::detail::is_single_threaded;

TEST_CASE("io_context_1" * doctest::timeout(1.))
{
  net::io_context ctx{1};
  CHECK(is_single_threaded(ctx));
  CHECK(is_single_threaded(ctx.get_executor()));
  CHECK(is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  CHECK(is_single_threaded(e2));
  CHECK(is_single_threaded(net::any_io_executor(e2)));
}

TEST_CASE("io_context_2" * doctest::timeout(1.))
{
  net::io_context ctx{2};
  CHECK(!is_single_threaded(ctx));
  CHECK(!is_single_threaded(ctx.get_executor()));
  CHECK(!is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  CHECK(!is_single_threaded(e2));
  CHECK(!is_single_threaded(net::any_io_executor(e2)));
}

TEST_CASE("thread_pool_1" * doctest::timeout(1.))
{
  net::thread_pool ctx{1};
  CHECK(is_single_threaded(ctx));
  CHECK(is_single_threaded(ctx.get_executor()));
  CHECK(is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  CHECK(is_single_threaded(e2));
  CHECK(is_single_threaded(net::any_io_executor(e2)));
}

TEST_CASE("thread_pool_2" * doctest::timeout(1.))
{
  net::thread_pool ctx{2};
  CHECK(!is_single_threaded(ctx));
  CHECK(!is_single_threaded(ctx.get_executor()));
  CHECK(!is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  CHECK(!is_single_threaded(e2));
  CHECK(!is_single_threaded(net::any_io_executor(e2)));
}

TEST_SUITE_END();