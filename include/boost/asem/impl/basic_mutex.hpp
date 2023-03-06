//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_IMPL_BASIC_MUTEX_HPP
#define BOOST_ASEM_IMPL_BASIC_MUTEX_HPP

#include <boost/asem/basic_mutex.hpp>
#include <boost/asem/detail/basic_op_model.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/deferred.hpp>
#include <asio/post.hpp>
#else
#include <boost/asio/deferred.hpp>
#include <boost/asio/post.hpp>
#endif


BOOST_ASEM_BEGIN_NAMESPACE

template < class Executor >
struct basic_mutex<Executor>::async_lock_op
{
    basic_mutex<Executor> * self;

    template< class Handler >
    void operator()(Handler &&handler)
    {
        auto e = get_associated_executor(handler, self->get_executor());
        auto l = self->impl_.internal_lock();
        ignore_unused(l);

        if (self->impl_.try_lock())
            return net::post(
                    std::move(e),
                    net::append(
                            std::forward< Handler >(handler), error_code()));

        using handler_type = std::decay_t< Handler >;
        using model_type = detail::basic_op_model< decltype(e), handler_type, void(error_code)>;
        model_type *model = model_type::construct(std::move(e), std::forward< Handler >(handler));

        auto slot = model->get_cancellation_slot();
        if (slot.is_connected())
        {
          auto &impl = self->impl_;
          slot.assign(
              [model, &impl](net::cancellation_type type)
              {
                if (type != net::cancellation_type::none)
                {
                  auto lock = impl.internal_lock();
                  ignore_unused(lock);
                  auto *self = model;
                  self->complete(net::error::operation_aborted);
                }
              });
        }
        self->impl_.add_waiter(model);
    }
};

BOOST_ASEM_END_NAMESPACE

#endif
