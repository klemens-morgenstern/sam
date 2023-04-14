// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP
#define BOOST_SAM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/basic_op.hpp>
#include <boost/sam/detail/semaphore_impl.hpp>

#include <condition_variable>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

semaphore_impl::semaphore_impl(net::execution_context &ctx,
                               int initial_count,
                               int concurrency_hint)
    : detail::service_member(ctx, concurrency_hint), count_(initial_count)
{
}

void semaphore_impl::add_waiter(detail::basic_op *waiter) noexcept { waiter->link_before(&waiters_); }

int semaphore_impl::count() const noexcept { return count_; }

void semaphore_impl::release()
{
  lock_type lock_{mtx_};;
  count_++;

  // release a pending operations
  if (waiters_.next_ == &waiters_)
    return;

  decrement();
  static_cast<detail::basic_op *>(waiters_.next_)->complete(std::error_code());
}

struct semaphore_impl::acquire_op_t final : detail::basic_op
{
  error_code   &ec;
  bool          done = false;
  detail::internal_condition_variable var;
  acquire_op_t(error_code &ec) : ec(ec) {}

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

void semaphore_impl::acquire(error_code &ec)
{
  if (!mtx_.enabled())
  {
    if (try_acquire())
      return;
    else
    {
      BOOST_SAM_ASSIGN_EC(ec, asio::error::in_progress);
      return;
    }
  }

  lock_type    lock{mtx_};
  acquire_op_t op{ec};
  add_waiter(&op);
  if (count_ > 0)
  {
    decrement();
    op.unlink();
    return;
  }
  op.wait(lock);
}

BOOST_SAM_NODISCARD int semaphore_impl::value() const
{
  lock_type lock_{mtx_};;
  if (waiters_.next_ == &waiters_)
    return count();

  return count() - static_cast<int>(waiters_.size());
}

bool semaphore_impl::try_acquire()
{
  lock_type _{mtx_};
  if (count_ > 0)
  {
    --count_;
    return true;
  }
  else
    return false;
}

int semaphore_impl::decrement()
{
  BOOST_SAM_ASSERT(count_ > 0);
  return --count_;
}

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP
