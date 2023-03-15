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

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(concurrency_hint);

using namespace BOOST_SAM_NAMESPACE;
using BOOST_SAM_NAMESPACE::detail::is_single_threaded;

BOOST_AUTO_TEST_CASE(io_context_1)
{
  net::io_context ctx{1};
  BOOST_TEST_CHECK(is_single_threaded(ctx));
  BOOST_TEST_CHECK(is_single_threaded(ctx.get_executor()));
  BOOST_TEST_CHECK(is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  BOOST_TEST_CHECK(is_single_threaded(e2));
  BOOST_TEST_CHECK(is_single_threaded(net::any_io_executor(e2)));
}

BOOST_AUTO_TEST_CASE(io_context_2)
{
  net::io_context ctx{2};
  BOOST_TEST_CHECK(!is_single_threaded(ctx));
  BOOST_TEST_CHECK(!is_single_threaded(ctx.get_executor()));
  BOOST_TEST_CHECK(!is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  BOOST_TEST_CHECK(!is_single_threaded(e2));
  BOOST_TEST_CHECK(!is_single_threaded(net::any_io_executor(e2)));
}

BOOST_AUTO_TEST_CASE(thread_pool_1)
{
  net::thread_pool ctx{1};
  BOOST_TEST_CHECK(is_single_threaded(ctx));
  BOOST_TEST_CHECK(is_single_threaded(ctx.get_executor()));
  BOOST_TEST_CHECK(is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  BOOST_TEST_CHECK(is_single_threaded(e2));
  BOOST_TEST_CHECK(is_single_threaded(net::any_io_executor(e2)));
}

BOOST_AUTO_TEST_CASE(thread_pool_2)
{
  net::thread_pool ctx{2};
  BOOST_TEST_CHECK(!is_single_threaded(ctx));
  BOOST_TEST_CHECK(!is_single_threaded(ctx.get_executor()));
  BOOST_TEST_CHECK(!is_single_threaded(net::any_io_executor(ctx.get_executor())));
  auto e2 = net::require(ctx.get_executor(), net::execution::outstanding_work.tracked);
  BOOST_TEST_CHECK(!is_single_threaded(e2));
  BOOST_TEST_CHECK(!is_single_threaded(net::any_io_executor(e2)));
}

BOOST_AUTO_TEST_SUITE_END();