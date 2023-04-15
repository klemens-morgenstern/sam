//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_LOCK_HPP
#define BOOST_SAM_LOCK_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_unique_lock.hpp>

BOOST_SAM_BEGIN_NAMESPACE

template <typename Executor>
struct basic_mutex;

template <typename Executor>
struct basic_shared_mutex;

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
basic_unique_lock<Executor> lock(basic_mutex<Executor> &mtx)
{
  mtx.lock();
  return basic_unique_lock<Executor>(mtx, std::adopt_lock);
}

/** Acquire a lock_guard synchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The lock_guard. It might be default constructed if locking  wasn't possible.
 */
template <typename Executor>
basic_unique_lock<Executor> lock(basic_mutex<Executor> &mtx, error_code &ec)
{
  mtx.lock(ec);
  if (ec)
    return basic_unique_lock<Executor>(mtx, std::defer_lock);
  else
    return basic_unique_lock<Executor>(mtx, std::adopt_lock);
}

template <typename Executor>
basic_unique_lock<Executor> lock(basic_shared_mutex<Executor> &mtx)
{
  mtx.lock();
  return basic_unique_lock<Executor>(mtx, std::adopt_lock);
}

template <typename Executor>
basic_unique_lock<Executor> lock(basic_shared_mutex<Executor> &mtx, error_code &ec)
{
  mtx.lock(ec);
  if (ec)
    return basic_unique_lock<Executor>(mtx, std::defer_lock);
  else
    return basic_unique_lock<Executor>(mtx, std::adopt_lock);
}

namespace detail
{

template<typename Mutex>
struct async_lock_op
{
  Mutex &mtx;

  template<typename Self>
  void operator()(Self && self)
  {
    mtx.async_lock(std::move(self));
  }

  template<typename Self>
  void operator()(Self && self, error_code ec)
  {
    if (ec)
      self.complete(ec, basic_unique_lock<typename Mutex::executor_type>{mtx, std::defer_lock});
    else
      self.complete(ec, basic_unique_lock<typename Mutex::executor_type>{mtx, std::adopt_lock});
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
          BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code, basic_unique_lock<Executor>))
              CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code, basic_unique_lock<Executor>))
async_lock(basic_mutex<Executor> &mtx, CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  return net::async_compose<
      CompletionToken, void(error_code, basic_unique_lock<Executor>)>
      (
          detail::async_lock_op<basic_mutex<Executor>>{mtx},
          token, mtx
      );
}

template <typename Executor,
          BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code, basic_unique_lock<Executor>))
              CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code, basic_unique_lock<Executor>))
async_lock(basic_shared_mutex<Executor> &mtx, CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  return net::async_compose<
      CompletionToken, void(error_code, basic_unique_lock<Executor>)>
      (
          detail::async_lock_op<basic_shared_mutex<Executor>>{mtx},
          token, mtx
      );
}

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_LOCK_HPP
