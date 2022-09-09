// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_IMPL_BASIC_SEMAPHORE_IPP
#define BOOST_ASEM_MT_IMPL_BASIC_SEMAPHORE_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/mt/basic_semaphore.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

semaphore_impl<mt>::semaphore_impl(int initial_count)
    : waiters_(), count_(initial_count)
{
}

void
semaphore_impl<mt>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

int
semaphore_impl<mt>::count() const noexcept
{
    return count_.load();
}

void
semaphore_impl<mt>::release()
{
    std::lock_guard<std::mutex> lock_{mtx_};
    count_ ++;

    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;

    decrement();
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}

BOOST_ASEM_NODISCARD int
semaphore_impl<mt>::value() const noexcept
{
    std::lock_guard<std::mutex> lock_{mtx_};
    if (waiters_.next_ == &waiters_)
        return count();

    return count() - static_cast<int>(waiters_.size());
}

bool
semaphore_impl<mt>::try_acquire()
{
    if (count_.fetch_sub(1) >= 0)
        return true;
    else
        count_--;
    return false;
}

int
semaphore_impl<mt>::decrement()
{
    //BOOST_ASEM_ASSERT(count_ > 0);
    return --count_;
}

BOOST_ASEM_DECL std::lock_guard<std::mutex> semaphore_impl<mt>::lock()
{
    return std::lock_guard<std::mutex>{mtx_};
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_SEMAPHORE_IPP
