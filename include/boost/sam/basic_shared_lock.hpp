//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_BASIC_SHARED_LOCK_HPP
#define BOOST_SAM_BASIC_SHARED_LOCK_HPP


#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/shared_mutex_impl.hpp>
#include <boost/sam/detail/exception.hpp>
#include <boost/sam/detail/async_lock_shared_mutex_op.hpp>

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
 * To use with with async_clock.
 */
template<typename Executor = net::any_io_executor>
struct basic_shared_lock
{
  using executor_type = net::any_io_executor;
  
  /// Construct an empty lock_guard.
  basic_shared_lock()                   = default;
  basic_shared_lock(const basic_shared_lock &) = delete;
  basic_shared_lock(basic_shared_lock &&lhs)
      : mtx_(lhs.mtx_), executor_(std::move(lhs.executor_)), locked_(lhs.locked_)
  {
    lhs.mtx_ = nullptr;
    lhs.locked_ = false;
  }

  template <typename Executor_>
  basic_shared_lock(basic_shared_lock<Executor_> &&lhs,
                    typename std::enable_if<std::is_convertible<Executor_, executor_type>::value>::type * = nullptr)
      : mtx_(lhs.mtx_), executor_(std::move(lhs.executor_)), locked_(lhs.locked_)
  {
    lhs.mtx_ = nullptr;
    lhs.locked_ = false;
  }

  basic_shared_lock &operator=(const basic_shared_lock &) = delete;
  basic_shared_lock &operator=(basic_shared_lock &&lhs)
  {
    std::swap(lhs.mtx_, mtx_);
    std::swap(lhs.locked_, locked_);
    std::swap(lhs.executor_, executor_);
    return *this;
  }

  template <typename Executor_>
  auto operator=(basic_shared_lock<Executor_> &&lhs)
      -> typename std::enable_if<std::is_convertible<Executor_, executor_type>::value, basic_shared_lock>::type &
  {
    std::swap(lhs.mtx_, mtx_);
    std::swap(lhs.locked_, locked_);
    std::swap(lhs.executor_, executor_);
    return *this;
  }

  /// Unlock the underlying mutex.
  ~basic_shared_lock()
  {
    if (mtx_ != nullptr && locked_)
      mtx_->unlock_shared();
  }

  template <typename Executor_>
  basic_shared_lock(basic_shared_mutex<Executor_> &mtx,
                    typename std::enable_if<std::is_convertible<Executor_, executor_type>::value>::type * = nullptr)
      : mtx_(&mtx.impl_), executor_(mtx.get_executor())
  {
    mtx.lock_shared();
    locked_ = true;
  }

  template <typename Executor_>
  basic_shared_lock(basic_shared_mutex<Executor_> &mtx,
                    typename std::enable_if<
                        std::is_convertible<Executor_, executor_type>::value,
                        std::defer_lock_t>::type) noexcept
      : mtx_(&mtx.impl_), executor_(mtx.get_executor())
  {
  }

  template <typename Executor_>
  basic_shared_lock(basic_shared_mutex<Executor_> &mtx,
                    typename std::enable_if<
                        std::is_convertible<Executor_, executor_type>::value,
                        std::adopt_lock_t>::type) noexcept
      : mtx_(&mtx.impl_), executor_(mtx.get_executor()), locked_(true)
  {
  }

  template <typename Executor_>
  basic_shared_lock(basic_shared_mutex<Executor_> &mtx,
                    typename std::enable_if<
                        std::is_convertible<Executor_, executor_type>::value,
                        std::try_to_lock_t>::type) noexcept
      : mtx_(&mtx.impl_), executor_(mtx.get_executor()), locked_(mtx.try_lock_shared())
  {
  }

  void unlock(error_code & ec)
  {
    if (mtx_ != nullptr && locked_)
      mtx_->unlock_shared();
    else if (mtx_ == nullptr)
      BOOST_SAM_ASSIGN_EC(ec, net::error::operation_not_supported);
    else
      BOOST_SAM_ASSIGN_EC(ec, net::error::no_permission);
  }
  void unlock()
  {
    error_code ec;
    unlock(ec);
    if (ec)
      detail::throw_error(ec, "unlock");
  }

  void release() {locked_ = false;}
  bool owns_lock() const noexcept { return locked_;}
  explicit operator bool () const noexcept { return locked_;}

  template <BOOST_SAM_COMPLETION_TOKEN_FOR(void(error_code))
                CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_SAM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
  async_lock(CompletionToken &&token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return net::async_initiate<CompletionToken, void(std::error_code)>(
        detail::async_lock_shared_mutex_op{mtx_}, token, get_executor(), locked_);
  }

  void lock(error_code & ec)
  {
    if (mtx_ == nullptr)
      BOOST_SAM_ASSIGN_EC(ec, net::error::operation_not_supported);
    else if (locked_)
      BOOST_SAM_ASSIGN_EC(ec, net::error::no_permission);
    else
      mtx_->lock_shared(ec);
  }
  void lock()
  {
    error_code ec;
    try_lock(ec);
    if (ec)
      detail::throw_error(ec, "lock");
  }

  bool try_lock(error_code & ec)
  {
    if (mtx_ == nullptr)
    {
      BOOST_SAM_ASSIGN_EC(ec, net::error::operation_not_supported);
      return false;
    }
    return mtx_->try_lock_shared();
  }
  bool try_lock()
  {
    error_code ec;
    bool res = try_lock(ec);
    if (ec)
      detail::throw_error(ec, "try_lock");
    return res;
  }

  executor_type get_executor() {return executor_;}

private:
  detail::shared_mutex_impl *mtx_ = nullptr;
  executor_type executor_;
  bool locked_ = false;

  template <typename>
  friend struct basic_shared_lock;
};

BOOST_SAM_END_NAMESPACE


#endif // BOOST_SAM_BASIC_SHARED_LOCK_HPP
