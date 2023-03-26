// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_IMPL_MUTEX_IMPL_IPP
#define BOOST_SAM_DETAIL_IMPL_MUTEX_IMPL_IPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/basic_op.hpp>
#include <boost/sam/detail/mutex_impl.hpp>

#include <condition_variable>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

void mutex_impl::add_waiter(detail::wait_op *waiter) noexcept { waiter->link_before(&waiters_); }

struct mutex_impl::lock_op_t final : detail::wait_op
{
  error_code   &ec;
  bool          done = false;
  detail::internal_condition_variable var;
  lock_op_t(error_code &ec) : ec(ec) {}

  void complete(error_code ec) override
  {
    done     = true;
    this->ec = ec;
    this->unlink();
    var.notify_all();
  }

  void shutdown() override
  {
    done = true;
    BOOST_SAM_ASSIGN_EC(this->ec, net::error::shut_down);
    this->unlink();
    var.notify_all();
  }

  void wait(lock_type &lock)
  {
    var.wait(lock, [this]{return done;});
  }
};

void mutex_impl::lock(error_code &ec)
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
  if (!locked_)
  {
    locked_ = true;
    op.unlink();
    return;
  }
  op.wait(lock);
}

void mutex_impl::unlock()
{
  lock_type lock{mtx_};
  // release a pending operations
  if (waiters_.next_ == &waiters_)
  {
    locked_ = false;
    return;
  }
  assert(waiters_.next_ != nullptr);
  static_cast<detail::wait_op *>(waiters_.next_)->complete(std::error_code());
}

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_DETAIL_IMPL_MUTEX_IMPL_IPP
