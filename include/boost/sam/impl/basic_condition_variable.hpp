// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_IMPL_BASIC_CONDITION_VARIABLE_HPP
#define BOOST_SAM_IMPL_BASIC_CONDITION_VARIABLE_HPP

#include <boost/sam/basic_condition_variable.hpp>
#include <boost/sam/detail/basic_op_model.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/dispatch.hpp>
#else
#include <boost/asio/dispatch.hpp>
#endif


BOOST_SAM_BEGIN_NAMESPACE

template <class Executor>
struct basic_condition_variable<Executor>::async_wait_op
{
  basic_condition_variable<Executor> *self;

  template <class Handler>
  void operator()(Handler &&handler)
  {
    auto e = get_associated_executor(handler, self->get_executor());
    detail::op_list_service::lock_type l{self->impl_.mtx_};
    ignore_unused(l);

    using handler_type   = typename std::decay<Handler>::type;
    using model_type     = detail::basic_op_model<decltype(e), handler_type>;
    model_type *model    = model_type::construct(std::move(e), std::forward<Handler>(handler));
    auto        slot     = model->get_cancellation_slot();
    if (slot.is_connected())
      slot.template emplace<detail::cancel_handler>(model, self->impl_.mtx_);
    self->impl_.add_waiter(model);
  }
};

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_IMPL_BASIC_CONDITION_VARIABLE_HPP
