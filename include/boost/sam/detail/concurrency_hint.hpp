// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_CONCURRENCY_HINT_HPP
#define BOOST_SAM_CONCURRENCY_HINT_HPP

#include <boost/sam/detail/config.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/any_io_executor.hpp>
#include <asio/io_context.hpp>
#else
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{
inline bool is_single_threaded(net::execution_context &ctx,
                               int concurrency_hint = BOOST_SAM_CONCURRENCY_HINT_DEFAULT)
{
  if (concurrency_hint != BOOST_SAM_CONCURRENCY_HINT_DEFAULT)
    return BOOST_SAM_CONCURRENCY_HINT_1 == concurrency_hint;

  const int cc = net::config(ctx).get("scheduler", "concurrency_hint", BOOST_SAM_CONCURRENCY_HINT_DEFAULT);
  return cc == BOOST_SAM_CONCURRENCY_HINT_1;
}

// only io_context or strands can be thread-safe
inline bool is_single_threaded(net::any_io_executor exec)
{
  using namespace net;
  return is_single_threaded(exec.context());
}

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_CONCURRENCY_HINT_HPP
