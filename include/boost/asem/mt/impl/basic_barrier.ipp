// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_IMPL_BASIC_BARRIER_IPP
#define BOOST_ASEM_MT_IMPL_BASIC_BARRIER_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/mt/basic_barrier.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

bool barrier_impl<mt>::try_arrive()
{
    if (counter_.fetch_sub(1) == 1)
    {
        auto l = lock();
        waiters_.complete_all({});
        counter_ = init_;
        return true;
    }
    else
        return false;
}

void
barrier_impl<mt>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}


}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_BARRIER_IPP
