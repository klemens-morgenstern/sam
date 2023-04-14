// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_IMPL_BARRIER_IMPL_IPP
#define BOOST_SAM_IMPL_BARRIER_IMPL_IPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_barrier.hpp>
#include <boost/sam/detail/basic_op.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

bool barrier_impl::try_arrive()
{
  lock_type _{mtx_};
  if (--counter_ == 0u)
  {
    waiters_.complete_all({});
    counter_ = init_;
    return true;
  }
  else
    counter_++;
  return false;
}

struct barrier_impl::arrive_op_t final : detail::basic_op
{
  error_code   &ec;
  bool          done = false;
  detail::internal_condition_variable var;
  arrive_op_t(error_code &ec) : ec(ec) {}

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

void barrier_impl::arrive(error_code &ec)
{
  if (!this->mtx_.enabled())
  {
    if (try_arrive())
      return;
    else
    {
      BOOST_SAM_ASSIGN_EC(ec, asio::error::in_progress);
      return;
    }
  }
  lock_type lock{this->mtx_};

  arrive_op_t op{ec};
  add_waiter(&op);
  // try_arrive
  counter_--;
  if (counter_ == 0u)
  {
    waiters_.complete_all({});
    counter_ = init_;
    op.unlink();
    return;
  }
  // try_arrive is over
  op.wait(lock);
}

void barrier_impl::add_waiter(detail::basic_op *waiter) noexcept { waiter->link_before(&waiters_); }

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_IMPL_BARRIER_IMPL_IPP
