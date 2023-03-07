// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP
#define BOOST_ASEM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/detail/semaphore_impl.hpp>

#include <condition_variable>

BOOST_ASEM_BEGIN_NAMESPACE
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
  if (multi_threaded())
    return ts_count_.load();
  else
    return count_;
}

void
semaphore_impl::release()
{
    auto lock_ = internal_lock();
    multi_threaded() ?
      ts_count_ ++ :
      count_ ++;

    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;

    decrement();
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}



void
semaphore_impl::acquire(error_code & ec)
{
    if (try_acquire())
        return ;
    else if (!multi_threaded())
    {
        BOOST_ASEM_ASSIGN_EC(ec, asio::error::in_progress);
        return ;
    }

    struct op_t final : detail::wait_op
    {
        error_code ec;
        semaphore_impl * this_;
        lock_type lock_{this_->internal_lock()};
        bool done = false;
        detail::event var;
        op_t(error_code & ec, semaphore_impl * this_) : ec(ec), this_(this_) {}

        void complete(error_code ec) override
        {
            done = true;
            this->ec = ec;
            var.signal_all(lock_);
            this_->decrement();
            this->unlink();
        }

        void shutdown() override
        {
          done = true;
          BOOST_ASEM_ASSIGN_EC(this->ec, net::error::shut_down);
          var.signal_all(lock_);
          this_->decrement();
          this->unlink();
        }

        ~op_t()
        {
        }

        void wait()
        {
            while (!done)
              var.wait(lock_);
        }
    };

    op_t op{ec, this};
    add_waiter(&op);
    op.wait();
}

BOOST_ASEM_NODISCARD int
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
  if (multi_threaded())
  {
    if (ts_count_.fetch_sub(1) >= 0)
      return true;
    else
      ts_count_++;
    return false;
  }
  else
  {
    bool acquired = false;
    if (count_ > 0)
    {
      --count_;
      acquired = true;
    }
    return acquired;
  }

}

int
semaphore_impl::decrement()
{
    //BOOST_ASEM_ASSERT(count_ > 0);
    return --count_;
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_DEDTAIL_IMPL_SEMAPHORE_IMPL_IPP
