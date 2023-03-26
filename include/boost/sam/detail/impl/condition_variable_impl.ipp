// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_IMPL_CONDITION_VARIABLE_IMPL_IPP
#define BOOST_SAM_DETAIL_IMPL_CONDITION_VARIABLE_IMPL_IPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/basic_op.hpp>
#include <boost/sam/detail/condition_variable_impl.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

condition_variable_impl::condition_variable_impl(net::execution_context &ctx) : detail::service_member(ctx) {}

void condition_variable_impl::add_waiter(detail::predicate_wait_op *waiter) noexcept { waiter->link_before(&waiters_); }

BOOST_SAM_DECL void condition_variable_impl::notify_one()
{
  lock_type lock{this->mtx_};
  // release a pending operations
  if (waiters_.next_ == &waiters_)
    return;
  auto op = static_cast<detail::predicate_wait_op *>(waiters_.next_);
  if (op->done())
    op->complete(error_code());
}

BOOST_SAM_DECL void condition_variable_impl::notify_all()
{
  lock_type lock{this->mtx_};
  // release a pending operations
  for (auto c = waiters_.next_; c != &waiters_;)
  {
    auto op = static_cast<detail::predicate_wait_op *>(c);
    c       = c->next_;
    if (op->done())
      op->complete(error_code());
  }
}

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_DETAIL_IMPL_CONDITION_VARIABLE_IMPL_IPP
