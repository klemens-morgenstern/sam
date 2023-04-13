//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_DETAIL_SHARED_MUTEX_IMPL_HPP
#define BOOST_SAM_DETAIL_SHARED_MUTEX_IMPL_HPP

#include <boost/sam/detail/basic_op_model.hpp>
#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/mutex_impl.hpp>
#include <boost/sam/detail/service.hpp>

#include <mutex>

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct shared_mutex_impl : mutex_impl
{
  using mutex_impl::mutex_impl;

  BOOST_SAM_DECL void lock(error_code &ec) override;
  bool                try_lock() override
  {
    lock_type _{mtx_};
    if (locked_ || locked_shared_ > 0u)
      return false;
    else
      return locked_ = true;
  }
  BOOST_SAM_DECL void unlock() override;
  bool is_locked() override { return locked_ || locked_shared_ != 0u; }

  BOOST_SAM_DECL void lock_shared(error_code &ec);
  bool                try_lock_shared()
  {
    lock_type _{mtx_};
    if (locked_)
      return false;
    else
    {
      locked_shared_++;
      return true;
    }
  }
  BOOST_SAM_DECL void unlock_shared();



  BOOST_SAM_DECL void add_shared_waiter(detail::wait_op *waiter) noexcept;

  void shutdown() override
  {
    lock_type l{mtx_};;
    auto w = std::move(waiters_);
    auto s = std::move(shared_waiters_);
    l.unlock();
    w.shutdown();
    s.shutdown();
  }

  std::uintptr_t locked_shared_{0u};
  detail::basic_bilist_holder<void(error_code)> shared_waiters_;

  shared_mutex_impl()                   = delete;
  shared_mutex_impl(const shared_mutex_impl &) = delete;
  shared_mutex_impl(shared_mutex_impl &&mi)
      : mutex_impl(std::move(mi)),
        locked_shared_(mi.locked_shared_),
        shared_waiters_(std::move(mi.shared_waiters_))
  {
    mi.locked_ = false;
  }

  shared_mutex_impl &operator=(const shared_mutex_impl &lhs) = delete;
  shared_mutex_impl &operator=(shared_mutex_impl &&lhs) noexcept
  {
    lock_type _{lhs.mtx_};
    locked_      = lhs.locked_;
    locked_shared_ = lhs.locked_shared_;
    lhs.locked_  = false;
    lhs.locked_shared_  = 0u;
    lhs.waiters_ = std::move(waiters_);
    lhs.shared_waiters_ = std::move(shared_waiters_);
    return *this;
  }
};

} // namespace detail

BOOST_SAM_END_NAMESPACE

#if defined(BOOST_SAM_HEADER_ONLY)
#include <boost/sam/detail/impl/mutex_impl.ipp>
#endif


#endif // BOOST_SAM_DETAIL_SHARED_MUTEX_IMPL_HPP
