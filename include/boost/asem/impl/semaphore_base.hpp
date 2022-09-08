//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/klemens-morgenstern/asem
//

#ifndef BOOST_ASEM_IMPL_BASIC_SEMAPHORE_BASE_HPP
#define BOOST_ASEM_IMPL_BASIC_SEMAPHORE_BASE_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/semaphore_wait_op.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

semaphore_base::semaphore_base(int initial_count)
: waiters_()
, count_(initial_count)
{
}

semaphore_base::~semaphore_base()
{
    auto & nx = waiters_.next_;
    while (nx != &waiters_)
         static_cast< detail::semaphore_wait_op * >(nx)->complete(BOOST_ASEM_ASIO_NAMESPACE::error::operation_aborted);
}

void
semaphore_base::add_waiter(detail::semaphore_wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

int
semaphore_base::count() const noexcept
{
    return count_;
}

void
semaphore_base::release()
{
    count_ += 1;

    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;

    decrement();
    static_cast< detail::semaphore_wait_op * >(waiters_.next_)
        ->complete(std::error_code());
}

std::size_t semaphore_base::release_all()
{
    std::size_t sz = 0u;
    auto & nx = waiters_.next_;
    while (nx != &waiters_)
    {
        static_cast< detail::semaphore_wait_op * >(nx)->complete(error_code());
        sz ++ ;
    }
    return sz;
}


BOOST_ASEM_NODISCARD inline int
semaphore_base::value() const noexcept
{
    if (waiters_.next_ == &waiters_)
        return count();

    return count() - static_cast<int>(waiters_.size());
}

bool
semaphore_base::try_acquire()
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
semaphore_base::decrement()
{
    BOOST_ASEM_ASSERT(count_ > 0);
    return --count_;
}

BOOST_ASEM_END_NAMESPACE

#endif
