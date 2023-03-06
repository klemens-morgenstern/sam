// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_DETAIL_IMPL_BASIC_CONDITION_VARIABLE_IPP
#define BOOST_ASEM_DETAIL_IMPL_BASIC_CONDITION_VARIABLE_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/detail/basic_condition_variable.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

condition_variable_impl::condition_variable_impl(
    net::execution_context & ctx) : detail::service_member(ctx)
{
}

void
condition_variable_impl::add_waiter(detail::predicate_wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

BOOST_ASEM_DECL void
condition_variable_impl::notify_one()
{
    auto lock = this->internal_lock();
    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;
    auto op = static_cast< detail::predicate_wait_op * >(waiters_.next_);
    if (op->done())
        op->complete(error_code());
}

BOOST_ASEM_DECL void
condition_variable_impl::notify_all()
{
  auto lock = this->internal_lock();
    // release a pending operations
    for (auto c = waiters_.next_; c != &waiters_;)
    {
        auto op = static_cast< detail::predicate_wait_op * >(c);
        c = c->next_;
        if (op->done())
            op->complete(error_code());
    }
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_CONDITION_VARIABLE_IPP
