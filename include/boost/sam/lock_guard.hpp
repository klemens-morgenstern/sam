// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_LOCK_GUARD_HPP
#define BOOST_SAM_LOCK_GUARD_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/mutex_impl.hpp>
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
struct basic_mutex;

/** A lock-guard used as an RAII object that automatically unlocks on destruction
 *
 * To use with with async_clock.
 */
struct lock_guard
{
  /// Construct an empty lock_guard.
  lock_guard()                   = default;
  lock_guard(const lock_guard &) = delete;
  lock_guard(lock_guard &&lhs) : mtx_(lhs.mtx_) { lhs.mtx_ = nullptr; }

  lock_guard &operator=(const lock_guard &) = delete;
  lock_guard &operator=(lock_guard &&lhs)
  {
    std::swap(lhs.mtx_, mtx_);
    return *this;
  }

  /// Unlock the underlying mutex.
  ~lock_guard()
  {
    if (mtx_ != nullptr)
      mtx_->unlock();
  }

  template <typename Executor>
  friend lock_guard lock(basic_mutex<Executor> &mtx);

  template <typename Executor>
  friend lock_guard lock(basic_mutex<Executor> &mtx, error_code &ec);

  template <typename Executor>
  lock_guard(basic_mutex<Executor> &mtx, const std::adopt_lock_t &) : mtx_(&mtx.impl_)
  {
  }

private:
  detail::mutex_impl *mtx_ = nullptr;
};

/** Acquire a lock_guard synchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The lock_guard.
 *
 * @throws May throw a system_error if locking is not possible without a deadlock.
 */
template <typename Executor>
lock_guard lock(basic_mutex<Executor> &mtx)
{
  mtx.lock();
  return lock_guard(std::adopt_lock, mtx);
}

/** Acquire a lock_guard synchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The lock_guard. It might be default constructed if locking  wasn't possible.
 */
template <typename Executor>
lock_guard lock(basic_mutex<Executor> &mtx, error_code &ec)
{
  mtx.lock(ec);
  if (ec)
    return lock_guard();
  else
    return lock_guard(mtx);
}

namespace detail
{

template<typename Executor>
struct async_lock_op
{
  basic_mutex<Executor> &mtx;

  template<typename Self>
  void operator()(Self && self)
  {
    mtx.async_lock(std::move(self));
  }

  template<typename Self>
  void operator()(Self && self, error_code ec)
  {
    if (ec)
      self.complete(ec, lock_guard{});
    else
      self.complete(ec, lock_guard{mtx, std::adopt_lock});
  }
};

}

/** Acquire a lock_guard asynchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The async_result deduced from the token.
 *
 * @tparam Implementation The mutex implementation
 * @tparam Executor The executor type of the mutex
 * @tparam CompletionToken The completion token.
 *
 * @example
 * @code{.cpp}
 *
 * awaitable<std::string> protected_read(st::mutex & mtx, tcp::socker & sock)
 * {
 *  std::string buf;
 *  auto l = co_await async_lock(mtx);
 *  co_await socket.async_read(dynamic_buffer(buf), use_awaitable);
 * }
 *
 * @endcode
 *
 */
template <typename Executor,
    BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code, lock_guard))
                                   CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code, lock_guard))
    async_lock(basic_mutex<Executor> &mtx, CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  return net::async_compose<
      CompletionToken, void(error_code, lock_guard)>
      (
          detail::async_lock_op<Executor>{mtx},
          token, mtx
      );
}

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_LOCK_GUARD_HPP
