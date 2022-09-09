// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_IMPL_BASIC_SEMAPHORE_IPP
#define BOOST_ASEM_ST_IMPL_BASIC_SEMAPHORE_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/st/basic_semaphore.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

semaphore_impl<st>::semaphore_impl(int initial_count)
    : waiters_(), count_(initial_count)
{
}

semaphore_impl<st>::~semaphore_impl()
{
    auto &nx = waiters_.next_;
    while (nx != &waiters_)
        static_cast< detail::wait_op * >(nx)->complete(BOOST_ASEM_ASIO_NAMESPACE::error::operation_aborted);
}

void
semaphore_impl<st>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

int
semaphore_impl<st>::count() const noexcept
{
    return count_;
}

void
semaphore_impl<st>::release()
{
    count_ += 1;

    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;

    decrement();
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}

BOOST_ASEM_NODISCARD int
semaphore_impl<st>::value() const noexcept
{
    if (waiters_.next_ == &waiters_)
        return count();

    return count() - static_cast<int>(waiters_.size());
}

bool
semaphore_impl<st>::try_acquire()
{
    bool acquired = false;
    if (count_ > 0)
    {
        --count_;
        acquired = true;
    }
    return acquired;
}

int
semaphore_impl<st>::decrement()
{
    BOOST_ASEM_ASSERT(count_ > 0);
    return --count_;
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_ST_IMPL_BASIC_SEMAPHORE_IPP
