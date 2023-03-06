// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_IMPL_BASIC_BARRIER_IPP
#define BOOST_ASEM_IMPL_BASIC_BARRIER_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/basic_barrier.hpp>
#include <condition_variable>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

bool barrier_impl::try_arrive()
{
  if (thread_safe())
  {
    if (ts_counter_.fetch_sub(1) <= 1)
    {
        auto l = internal_lock();
        waiters_.complete_all({});
        ts_counter_ = init_;
        return true;
    }
    else
    {
        ts_counter_ ++;
        return false;
    }
  }
  else
  {
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
}

void barrier_impl::arrive(error_code &ec)
{
    if (try_arrive())
        return;
    else if (!thread_safe())
    {
      BOOST_ASEM_ASSIGN_EC(ec, asio::error::in_progress);
      return ;
    }

    struct op_t final : detail::wait_op
    {
        error_code ec;
        bool done = false;
        detail::event var;
        lock_type &lock;
        op_t(error_code & ec,
             lock_type & lock) : ec(ec), lock(lock) {}

        void complete(error_code ec) override
        {
            done = true;
            this->ec = ec;
            var.signal_all(lock);
            this->unlink();
        }

        void shutdown() override
        {
          done = true;
          BOOST_ASEM_ASSIGN_EC(this->ec, net::error::shut_down);
          var.signal_all(lock);
          this->unlink();
        }

        ~op_t()
        {
        }

        void wait()
        {
            while (!done)
                var.wait(lock);
        }
    };
    auto lock = this->internal_lock();
    op_t op{ec, lock};
    decrement();
    add_waiter(&op);
    op.wait();
}

void
barrier_impl::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}


}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_IMPL_BASIC_BARRIER_IPP
