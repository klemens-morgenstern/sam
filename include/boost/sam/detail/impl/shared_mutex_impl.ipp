//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_DETAIL_IMPL_SHARED_MUTEX_IMPL_IPP
#define BOOST_SAM_DETAIL_IMPL_SHARED_MUTEX_IMPL_IPP

#include <boost/sam/detail/shared_mutex_impl.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

void shared_mutex_impl::add_shared_waiter(detail::wait_op *waiter) noexcept { waiter->link_before(&shared_waiters_); }

void shared_mutex_impl::lock(error_code &ec)
{
  if (!this->mtx_.enabled())
  {
    if (try_lock())
      return;
    else
    {
      BOOST_SAM_ASSIGN_EC(ec, asio::error::in_progress);
      return;
    }
  }

  lock_type lock{mtx_};
  lock_op_t op{ec};
  add_waiter(&op);
  if (!locked_ && locked_shared_ == 0u)
  {
    locked_ = true;
    op.unlink();
    return;
  }
  op.wait(lock);
}

void shared_mutex_impl::unlock()
{
  lock_type lock{mtx_};
  if (shared_waiters_.next_ != &shared_waiters_)
  {
    // unlock unique lock
    locked_ = false;
    while (shared_waiters_.next_ != &shared_waiters_)
    {
      locked_shared_++;
      static_cast<detail::wait_op *>(shared_waiters_.next_)->complete(std::error_code());
    }
    return ;
  }

  // release a pending operations
  if (waiters_.next_ == &waiters_)
  {
    locked_ = false;
    return;
  }
  assert(waiters_.next_ != nullptr);
  static_cast<detail::wait_op *>(waiters_.next_)->complete(std::error_code());
}


void shared_mutex_impl::lock_shared(error_code &ec)
{
  if (!this->mtx_.enabled())
  {
    if (try_lock_shared())
      return;
    else
    {
      BOOST_SAM_ASSIGN_EC(ec, asio::error::in_progress);
      return;
    }
  }

  lock_type lock{mtx_};
  lock_op_t op{ec};
  add_waiter(&op);
  if (!locked_)
  {
    locked_shared_++;
    op.unlink();
    return;
  }
  op.wait(lock);
}

void shared_mutex_impl::unlock_shared()
{
  lock_type lock{mtx_};

  if (locked_shared_ > 0u)
    locked_shared_--;
  if (locked_shared_ == 0u)
  {
    if (waiters_.next_ != &waiters_)
    {
      locked_ = true;
      static_cast<detail::wait_op *>(waiters_.next_)->complete(std::error_code());
    }

  }
}


}
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_DETAIL_IMPL_SHARED_MUTEX_IMPL_IPP
