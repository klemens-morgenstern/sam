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

semaphore_impl::semaphore_impl(net::execution_context & ctx, int initial_count)
    : detail::service_member(ctx), count_(initial_count)
{
}

void
semaphore_impl::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

int
semaphore_impl::count() const noexcept
{
  return count_;
}

void
semaphore_impl::release()
{
    auto lock_ = internal_lock();
    count_ ++;

    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;

    decrement();
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}

struct semaphore_impl::acquire_op_t final : detail::wait_op
{
  error_code &ec;
  lock_type & lock_;
  bool done = false;
  detail::event var;
  acquire_op_t(error_code & ec,
               lock_type & lock_) : ec(ec), lock_(lock_) {}

  void complete(error_code ec) override
  {
    done = true;
    this->ec = ec;
    var.signal_all(lock_);
    this->unlink();
  }

  void shutdown() override
  {
    done = true;
    BOOST_SAM_ASSIGN_EC(this->ec, net::error::shut_down);
    var.signal_all(lock_);
    this->unlink();
  }

  void wait()
  {
    while (!done)
      var.wait(lock_);
  }
};

void
semaphore_impl::acquire(error_code & ec)
{
    if (!mtx_.enabled())
    {
        if (try_acquire())
          return;
        else
        {
          BOOST_SAM_ASSIGN_EC(ec, asio::error::in_progress);
          return ;
        }
    }

    lock_type lock = internal_lock();
    acquire_op_t op{ec, lock};
    add_waiter(&op);
    if (count_ > 0)
    {
        decrement();
        op.unlink();
        return;
    }
    op.wait();
}

BOOST_SAM_NODISCARD int
semaphore_impl::value() const noexcept
{
    auto lock_ = internal_lock();
    if (waiters_.next_ == &waiters_)
        return count();

    return count() - static_cast<int>(waiters_.size());
}

bool
semaphore_impl::try_acquire()
{
  auto _ = internal_lock();
  if (count_ > 0)
  {
    --count_;
    return true;
  }
  else
    return false;
}

int
semaphore_impl::decrement()
{
    BOOST_SAM_ASSERT(count_ > 0);
    return --count_;
}

}
BOOST_SAM_END_NAMESPACE


#endif //BOOST_SAM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP
