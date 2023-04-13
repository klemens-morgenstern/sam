//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_SAM_DETAILL_ASYNC_LOCK_MUTEX_OP_HPP
#define BOOST_SAM_DETAILL_ASYNC_LOCK_MUTEX_OP_HPP

#include <boost/sam/detail/basic_op_model.hpp>
#include <boost/sam/detail/mutex_impl.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/associated_immediate_executor.hpp>
#include <asio/cancellation_signal.hpp>
#include <asio/dispatch.hpp>
#else
#include <boost/asio/associated_immediate_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/dispatch.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct async_lock_mutex_op
{
  detail::mutex_impl &impl_;

  template <typename Handler, typename Executor>
  void operator()(Handler &&handler, Executor executor)
  {
    auto e = get_associated_executor(handler, executor);
    detail::op_list_service::lock_type l{impl_.mtx_};
    ignore_unused(l);

    if (!impl_.is_locked())
    {
      impl_.locked_ = true;
      auto ie             = net::get_associated_immediate_executor(handler, executor);
      return net::dispatch(ie, net::append(std::forward<Handler>(handler), error_code()));
    }
    using handler_type = typename std::decay<Handler>::type;
    using model_type   = detail::basic_op_model<decltype(e), handler_type, void(error_code)>;
    model_type *model  = model_type::construct(std::move(e), std::forward<Handler>(handler));

    auto slot = model->get_cancellation_slot();
    if (slot.is_connected())
    {
      auto &impl = impl_;
      slot.assign(
          [model, &impl](net::cancellation_type type)
          {
            if (type != net::cancellation_type::none)
            {
              detail::op_list_service::lock_type lock{impl.mtx_};
              ignore_unused(lock);
              auto *self = model;
              self->complete(net::error::operation_aborted);
            }
          });
    }
    impl_.add_waiter(model);
  }
};

}

BOOST_SAM_END_NAMESPACE

#endif
