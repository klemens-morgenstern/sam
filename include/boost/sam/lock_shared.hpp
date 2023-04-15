//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_LOCK_SHARED_HPP
#define BOOST_SAM_LOCK_SHARED_HPP


#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_shared_lock.hpp>
#include <boost/sam/shared_lock.hpp>
#include <mutex>
#include <utility>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/async_result.hpp>
#include <asio/compose.hpp>
#include <asio/deferred.hpp>

#else
#include <boost/asio/async_result.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/deferred.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

template <typename Executor>
shared_lock lock_shared(basic_shared_mutex<Executor> &mtx)
{
  mtx.lock_shared();
  return shared_lock(mtx, std::adopt_lock);
}

template <typename Executor>
shared_lock lock_shared(basic_shared_mutex<Executor> &mtx, error_code &ec)
{
  mtx.lock_shared(ec);
  if (ec)
    return shared_lock(mtx, std::defer_lock);
  else
    return shared_lock(mtx, std::adopt_lock);
}

namespace detail
{

template<typename Mutex>
struct async_lock_shared_op
{
  Mutex &mtx;

  template<typename Self>
  void operator()(Self && self)
  {
    mtx.async_lock_shared(std::move(self));
  }

  template<typename Self>
  void operator()(Self && self, error_code ec)
  {
    if (ec)
      self.complete(ec, shared_lock{mtx, std::defer_lock});
    else
      self.complete(ec, shared_lock{mtx, std::adopt_lock});
  }
};


}

template <typename Executor,
          BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code, basic_shared_lock<Executor>))
              CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code, basic_shared_lock<Executor>))
async_lock_shared(basic_shared_mutex<Executor> &mtx, CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  return net::async_compose<
      CompletionToken, void(error_code, basic_shared_lock<Executor>)>
      (
          detail::async_lock_shared_op<basic_shared_mutex<Executor>>{mtx},
          token, mtx
      );
}

BOOST_SAM_END_NAMESPACE


#endif // BOOST_SAM_LOCK_SHARED_HPP
