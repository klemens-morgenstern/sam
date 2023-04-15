// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_BASIC_MUTEX_HPP
#define BOOST_SAM_BASIC_MUTEX_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/exception.hpp>
#include <boost/sam/detail/mutex_impl.hpp>

#include <boost/sam/detail/async_lock_mutex_op.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/any_io_executor.hpp>
#include <asio/compose.hpp>
#else
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/compose.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

/** An asio based mutex modeled on `std::mutex`.
 *
 * @tparam Implementation The implementation, st or mt.
 * @tparam Executor The executor to use as default completion.
 */
template <typename Executor = net::any_io_executor>
struct basic_mutex
{
  /// The executor type.
  using executor_type = Executor;

  /// A constructor. @param exec The executor to be used by the mutex.
  explicit basic_mutex(executor_type exec,
                       int concurrency_hint = BOOST_SAM_CONCURRENCY_HINT_DEFAULT)
    : exec_(std::move(exec)), impl_{net::query(exec_, net::execution::context), concurrency_hint}
  {
  }

  /// A constructor. @param ctx The execution context used by the mutex.
  template <typename ExecutionContext>
  explicit basic_mutex(ExecutionContext &ctx,
          typename std::enable_if<std::is_convertible<ExecutionContext &, net::execution_context &>::value, int>::type
          concurrency_hint = BOOST_SAM_CONCURRENCY_HINT_DEFAULT)
      : exec_(ctx.get_executor()), impl_(ctx, concurrency_hint)
  {
  }

  /// @brief Rebind a mutex to a new executor - this cancels all outstanding operations.
  template <typename Executor_>
  basic_mutex(basic_mutex<Executor_> &&sem,
              typename std::enable_if<std::is_convertible<Executor_, executor_type>::value>::type * = nullptr)
      : exec_(sem.get_executor()), impl_(std::move(sem.impl_))
  {
  }

  /** Wait for the mutex to become lockable & lock it.
   *
   * @tparam CompletionToken The completion token type.
   * @param token The token for completion.
   * @return Deduced from the token.
   */
  template <BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code))
                CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
  async_lock(CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return net::async_initiate<CompletionToken, void(std::error_code)>(
        detail::async_lock_mutex_op{&impl_}, token, get_executor());
  }

  /// Move assign a mutex.
  basic_mutex &operator=(basic_mutex &&) noexcept = default;

  /// Move assign a mutex with a different executor.
  template <typename Executor_>
  auto operator=(basic_mutex<Executor_> &&sem)
      -> typename std::enable_if<std::is_convertible<Executor_, executor_type>::value, basic_mutex>::type &
  {
    std::swap(exec_, sem.exec_);
    std::swap(impl_, sem.impl_);
    return *this;
  }

  basic_mutex &operator=(const basic_mutex &) = delete;

  /** Lock synchronously. This may fail depending on the implementation.
   *
   * If the implementation is `st` this will generate an error if the mutex
   * is already locked.
   *
   * If the implementation is `mt` this function will block until another thread releases
   * the lock. Note that this may lead to deadlocks.
   *
   * You should never use the synchronous functions from within an asio event-queue.
   *
   */
  void lock(error_code &ec) { impl_.lock(ec); }

  /// Throwing @overload lock(error_code &);
  void lock()
  {
    error_code ec;
    lock(ec);
    if (ec)
      detail::throw_error(ec, "lock");
  }
  /// Unlock the mutex, and complete one pending lock if pending.
  void unlock() { impl_.unlock(); }

  ///  Try to lock the mutex.
  bool try_lock() { return impl_.try_lock(); }

  /// Rebinds the mutex type to another executor.
  template <typename Executor1>
  struct rebind_executor
  {
    /// The mutex type when rebound to the specified executor.
    typedef basic_mutex<Executor1> other;
  };

  /// @brief return the default executor.
  executor_type get_executor() const noexcept { return exec_; }

private:
  template <typename>
  friend struct basic_mutex;
  template <typename>
  friend struct basic_unique_lock;
  template <typename>
  friend struct basic_shared_lock;

  Executor           exec_;
  detail::mutex_impl impl_;
};

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_BASIC_MUTEX_HPP
