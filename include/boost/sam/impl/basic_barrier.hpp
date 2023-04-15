//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_SAM_IMPL_BASIC_BARRIER_HPP
#define BOOST_SAM_IMPL_BASIC_BARRIER_HPP

#include <boost/sam/basic_barrier.hpp>
#include <boost/sam/detail/basic_op_model.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/associated_immediate_executor.hpp>
#include <asio/deferred.hpp>
#include <asio/dispatch.hpp>
#include <asio/post.hpp>
#else
#include <boost/asio/associated_immediate_executor.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

template <class Executor>
struct basic_barrier<Executor>::async_arrive_op
{
  basic_barrier<Executor> *self;

  template <class Handler>
  void operator()(Handler &&handler)
  {
    auto e = get_associated_executor(handler, self->get_executor());

    if (self->impl_.try_arrive())
    {
      auto ie = net::get_associated_immediate_executor(handler, self->get_executor());
      return net::dispatch(ie, net::append(std::forward<Handler>(handler), error_code()));
    }

    detail::op_list_service::lock_type l{self->impl_.mtx_};
    self->impl_.decrement();
    ignore_unused(l);
    using handler_type = typename std::decay<Handler>::type;
    using model_type   = detail::basic_op_model<decltype(e), handler_type>;
    model_type *model  = model_type::construct(std::move(e), std::forward<Handler>(handler));

    auto slot = model->get_cancellation_slot();
    if (slot.is_connected())
      slot.template emplace<detail::cancel_handler>(model, self->impl_.mtx_);

    self->impl_.add_waiter(model);
  }
};

BOOST_SAM_END_NAMESPACE

#endif
