// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_IMPL_BASIC_BARRIER_IPP
#define BOOST_ASEM_ST_IMPL_BASIC_BARRIER_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/st/basic_barrier.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

bool barrier_impl<st>::try_arrive()
{
    counter_--;
    if (counter_ == 0u)
    {
        waiters_.complete_all({});
        counter_ = init_;
        return true;
    }
    return false;
}

void
barrier_impl<st>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_ST_IMPL_BASIC_BARRIER_IPP
