// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_IMPL_BASIC_CONDITION_VARIABLE_HPP
#define BOOST_SAM_IMPL_BASIC_CONDITION_VARIABLE_HPP

#include <boost/sam/basic_condition_variable.hpp>
#include <boost/sam/detail/predicate_op_model.hpp>

BOOST_SAM_BEGIN_NAMESPACE

template <class Executor>
template <class Predicate>
struct basic_condition_variable<Executor>::async_predicate_wait_op
{
  basic_condition_variable<Executor> *self;
  Predicate                           predicate;
  template <class Handler>
  void operator()(Handler &&handler)
  {
    auto e = get_associated_executor(handler, self->get_executor());
    auto l = self->impl_.internal_lock();
    ignore_unused(l);

    using handler_type   = std::decay_t<Handler>;
    using predicate_type = Predicate;
    using model_type     = detail::predicate_op_model<decltype(e), handler_type, predicate_type, void(error_code)>;
    model_type *model    = model_type::construct(std::move(e), std::forward<Handler>(handler), std::move(predicate));

    auto slot = model->get_cancellation_slot();
    if (slot.is_connected())
    {
      auto &impl = self->impl_;
      slot.assign(
          [model, &impl, slot](net::cancellation_type type)
          {
            if (type != net::cancellation_type::none)
            {
              auto sl   = slot;
              auto lock = impl.internal_lock();
              ignore_unused(lock);
              // completed already
              if (!sl.is_connected())
                return;

              auto *self = model;
              self->complete(net::error::operation_aborted);
            }
          });
    }

    self->impl_.add_waiter(model);
  }
};

template <class Executor>
struct basic_condition_variable<Executor>::async_wait_op
{
  basic_condition_variable<Executor> *self;

  struct true_predicate
  {
    constexpr bool operator()() const noexcept { return true; }
  };

  template <class Handler>
  void operator()(Handler &&handler)
  {
    auto e = get_associated_executor(handler, self->get_executor());
    auto l = self->impl_.internal_lock();
    ignore_unused(l);

    using handler_type   = std::decay_t<Handler>;
    using predicate_type = true_predicate;
    using model_type     = detail::predicate_op_model<decltype(e), handler_type, predicate_type, void(error_code)>;
    model_type *model    = model_type::construct(std::move(e), std::forward<Handler>(handler), true_predicate{});
    auto        slot     = model->get_cancellation_slot();
    if (slot.is_connected())
    {
      auto &impl = self->impl_;
      slot.assign(
          [model, &impl, slot](net::cancellation_type type)
          {
            if (type != net::cancellation_type::none)
            {
              auto sl   = slot;
              auto lock = impl.internal_lock();
              ignore_unused(lock);
              // completed already
              if (!sl.is_connected())
                return;

              auto *self = model;
              self->complete(net::error::operation_aborted);
            }
          });
    }
    self->impl_.add_waiter(model);
  }
};

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_IMPL_BASIC_CONDITION_VARIABLE_HPP
