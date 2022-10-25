// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_IMPL_BASIC_MUTEX_IPP
#define BOOST_ASEM_MT_IMPL_BASIC_MUTEX_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/mt/basic_mutex.hpp>

#include <condition_variable>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

void
mutex_impl<mt>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

void
mutex_impl<mt>::lock(error_code & ec)
{
    if (try_lock())
        return ;
    struct op_t final : detail::wait_op
    {
        error_code ec;
        bool done = false;
        std::condition_variable var;
        op_t(error_code & ec) : ec(ec) {}

        void complete(error_code ec) override
        {
            done = true;
            this->ec = ec;
            var.notify_all();
            this->unlink();
        }

        void wait(std::unique_lock<std::mutex> & lock)
        {
            var.wait(lock, [this]{ return done;});
        }
    };

    op_t op{ec};
    std::unique_lock<std::mutex> lock(mtx_);
    add_waiter(&op);
    op.wait(lock);
}

void
mutex_impl<mt>::unlock()
{
    std::lock_guard<std::mutex> lock_{mtx_};

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
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_MUTEX_IPP
