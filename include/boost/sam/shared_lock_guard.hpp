//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_SHARED_LOCK_GUARD_HPP
#define BOOST_SAM_SHARED_LOCK_GUARD_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/shared_mutex_impl.hpp>
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
struct basic_shared_mutex;

/** A lock-guard used as an RAII object that automatically unlocks on destruction
 *
 * To use with with async_lock.
 */
struct shared_lock_guard
{
  /// Construct an empty shared_lock_guard.
  shared_lock_guard()                   = default;
  shared_lock_guard(const shared_lock_guard &) = delete;
  shared_lock_guard(shared_lock_guard &&lhs) : mtx_(lhs.mtx_) { lhs.mtx_ = nullptr; }

  shared_lock_guard &operator=(const shared_lock_guard &) = delete;
  shared_lock_guard &operator=(shared_lock_guard &&lhs)
  {
    std::swap(lhs.mtx_, mtx_);
    return *this;
  }

  /// Unlock the underlying mutex.
  ~shared_lock_guard()
  {
    if (mtx_ != nullptr)
      mtx_->unlock_shared();
  }

  template <typename Executor>
  shared_lock_guard(basic_shared_mutex<Executor> &mtx, const std::adopt_lock_t &) : mtx_(&mtx.impl_)
  {
  }

private:
  detail::shared_mutex_impl *mtx_ = nullptr;
};

template <typename Executor>
shared_lock_guard lock_shared(basic_shared_mutex<Executor> &mtx)
{
  mtx.lock_shared();
  return shared_lock_guard(mtx, std::adopt_lock);
}

template <typename Executor>
shared_lock_guard lock_shared(basic_shared_mutex<Executor> &mtx, error_code &ec)
{
  mtx.lock_shared(ec);
  if (ec)
    return shared_lock_guard();
  else
    return shared_lock_guard(mtx, std::adopt_lock);
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
      self.complete(ec, shared_lock_guard{});
    else
      self.complete(ec, shared_lock_guard{mtx, std::adopt_lock});
  }
};


}

template <typename Executor,
          BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code, shared_lock_guard))
              CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code, shared_lock_guard))
async_lock_shared(basic_shared_mutex<Executor> &mtx, CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  return net::async_compose<
      CompletionToken, void(error_code, shared_lock_guard)>
      (
          detail::async_lock_shared_op<basic_shared_mutex<Executor>>{mtx},
          token, mtx
      );
}

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_SHARED_LOCK_GUARD_HPP
