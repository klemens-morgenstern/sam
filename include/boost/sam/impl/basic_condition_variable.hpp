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

namespace detail
{

template<std::size_t I> struct rank : rank<I - 1> {};
template<>              struct rank<0> {};

template<typename Lock, typename Handler, typename Executor>
void do_lockable_complete(error_code & ec, Lock * lock,
                          Handler &&h, Executor exec, rank<0u>)
{
  if (!ec)
    lock->lock();
  net::dispatch(exec, net::append(std::move(h), ec));
}

template<typename Lock, typename Handler, typename Executor>
auto do_lockable_complete(error_code & ec, Lock * lock,
                          Handler &&h, Executor exec, rank<1u>) -> decltype(lock->lock(ec))
{
  if (!ec)
    lock->lock(ec);
  net::dispatch(exec, net::append(std::move(h), ec));
}

template<typename Lock, typename Handler, typename Executor>
auto do_lockable_complete(error_code & ec, Lock * lock,
                          Handler &&h, Executor exec, rank<2u>)
  -> decltype(lock->async_lock(std::move(h)))
{
  if (!ec)
    lock->async_lock(std::move(h));
  else
    net::dispatch(exec, net::append(std::move(h), ec));
}


template<typename Lock>
auto do_unlock(error_code &, Lock * lock, rank<0u>) -> decltype(lock->unlock())
{
  lock->unlock();
}

template<typename Lock>
auto do_unlock(error_code & ec, Lock * lock, rank<1u>) -> decltype(lock->unlock(ec))
{
  lock->unlock(ec);
}


template <class Executor, class Handler, class Lock>
struct lockable_op_model final : basic_op<void(error_code)>
{
  using executor_type           = Executor;
  using immediate_executor_type = Executor;
  using cancellation_slot_type  = net::associated_cancellation_slot_t<Handler>;
  using allocator_type          = net::associated_allocator_t<Handler>;

  allocator_type get_allocator() { return net::get_associated_allocator(handler_); }

  cancellation_slot_type get_cancellation_slot() { return net::get_associated_cancellation_slot(handler_); }

  executor_type get_executor() { return work_guard_.get_executor(); }
  executor_type get_immediate_executor() { return work_guard_.get_executor(); }

  static lockable_op_model *construct(Executor e, Handler handler, Lock * lock)
  {
    auto halloc  = net::get_associated_allocator(handler);
    auto alloc   = typename std::allocator_traits<decltype(halloc)>::template rebind_alloc<lockable_op_model>(halloc);
    using traits = std::allocator_traits<decltype(alloc)>;
    auto pmem    = traits::allocate(alloc, 1);

    try
    {
      return new (pmem) lockable_op_model(std::move(e), std::move(handler), lock);
    }
    catch (...)
    {
      traits::deallocate(alloc, pmem, 1);
      throw;
    }
  }

  static void destroy(lockable_op_model *self, net::associated_allocator_t<Handler> halloc)
  {
    auto alloc = typename std::allocator_traits<decltype(halloc)>::template rebind_alloc<lockable_op_model>(halloc);
    self->~lockable_op_model();
    auto traits = std::allocator_traits<decltype(alloc)>();
    traits.deallocate(alloc, self, 1);
  }

  lockable_op_model(Executor e, Handler handler, Lock * lock)
      : work_guard_(std::move(e)), handler_(std::move(handler)), lock_(lock) {}

  virtual void complete(error_code ec) override
  {
    get_cancellation_slot().clear();
    auto g = std::move(work_guard_);
    auto h = std::move(handler_);
    auto lock = lock_;
    this->unlink();
    destroy(this, net::get_associated_allocator(h));
    do_lockable_complete(ec, lock, std::move(h), g.get_executor(), rank<2u>{});
  }

  virtual void shutdown() override
  {
    get_cancellation_slot().clear();
    this->unlink();
    destroy(this, net::get_associated_allocator(this->handler_));
  }

private:
  net::executor_work_guard<Executor> work_guard_;
  Handler                            handler_;
  Lock                              *lock_;
};


}

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
    using model_type     = detail::basic_op_model<decltype(e), handler_type, void(error_code)>;
    model_type *model    = model_type::construct(std::move(e), std::forward<Handler>(handler));
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
              detail::op_list_service::lock_type lock{impl.mtx_};
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
