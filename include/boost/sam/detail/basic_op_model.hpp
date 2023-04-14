//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_SAM_DETAIL_SEMAPHORE_WAIT_OP_MODEL_HPP
#define BOOST_SAM_DETAIL_SEMAPHORE_WAIT_OP_MODEL_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/basic_op.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/associated_allocator.hpp>
#include <asio/associated_cancellation_slot.hpp>
#include <asio/executor_work_guard.hpp>
#else
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/executor_work_guard.hpp>
#endif


BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{
template <class Executor, class Handler>
struct basic_op_model;

template <class Executor, class Handler>
struct basic_op_model final : basic_op
{
  using executor_type          = Executor;
  using cancellation_slot_type = net::associated_cancellation_slot_t<Handler>;
  using allocator_type         = net::associated_allocator_t<Handler>;

  allocator_type get_allocator() { return net::get_associated_allocator(handler_); }

  cancellation_slot_type get_cancellation_slot() { return net::get_associated_cancellation_slot(handler_); }

  executor_type get_executor() { return work_guard_.get_executor(); }

  static basic_op_model *construct(Executor e, Handler handler);

  static void destroy(basic_op_model *self, net::associated_allocator_t<Handler> halloc);

  basic_op_model(Executor e, Handler handler);

  virtual void complete(error_code ec) override;
  virtual void shutdown() override;

private:
  net::executor_work_guard<Executor> work_guard_;
  Handler                            handler_;
};

} // namespace detail

BOOST_SAM_END_NAMESPACE

#endif

#include <boost/sam/detail/impl/basic_op_model.hpp>
