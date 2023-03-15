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

void
mutex_impl::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

void
mutex_impl::lock(error_code & ec)
{
    if (try_lock())
        return ;
    else if (!this->mtx_.enabled())
    {
        BOOST_SAM_ASSIGN_EC(ec, asio::error::in_progress);
        return ;
    }
    struct op_t final : detail::wait_op
    {
        error_code &ec;
        std::atomic<bool> done = false;
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
          BOOST_SAM_ASSIGN_EC(this->ec, net::error::shut_down);
          var.signal_all(lock);
          this->unlink();
        }

        void wait()
        {
          while (!done)
            var.wait(lock);
        }
    };

    auto lock = this->internal_lock();
    op_t op{ec, lock};
    add_waiter(&op);
    op.wait();
}

void
mutex_impl::unlock()
{
    auto lock = internal_lock();

    // release a pending operations
    if (waiters_.next_ == &waiters_)
    {
        locked_ = false;
        return;
    }
    assert(waiters_.next_ != nullptr);
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}

}
BOOST_SAM_END_NAMESPACE


#endif //BOOST_SAM_DETAIL_IMPL_MUTEX_IMPL_IPP
