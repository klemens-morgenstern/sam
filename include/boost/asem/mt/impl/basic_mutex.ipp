// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_IMPL_BASIC_MUTEX_IPP
#define BOOST_ASEM_MT_IMPL_BASIC_MUTEX_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/mt/basic_mutex.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

void
mutex_impl<mt>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
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
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_MUTEX_IPP
