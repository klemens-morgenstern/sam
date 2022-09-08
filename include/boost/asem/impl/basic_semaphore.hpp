//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_IMPL_BASIC_ASYNC_SEMAPHORE_HPP
#define BOOST_ASEM_IMPL_BASIC_ASYNC_SEMAPHORE_HPP

#include <boost/asem/basic_semaphore.hpp>
#include <boost/asem/detail/semaphore_wait_op_model.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/post.hpp>
#else
#include <boost/asio/post.hpp>
#endif


BOOST_ASEM_BEGIN_NAMESPACE

template < class Executor >
basic_semaphore< Executor >::basic_semaphore(executor_type exec,
                                                         int initial_count)
: semaphore_base(initial_count)
, exec_(std::move(exec))
{
}

template < class Executor >
typename basic_semaphore< Executor >::executor_type const &
basic_semaphore< Executor >::get_executor() const
{
    return exec_;
}


template < class Executor >
struct basic_semaphore<Executor>::async_aquire_op
{
    basic_semaphore<Executor> * self;

    template< class Handler >
    void operator()(Handler &&handler)
    {
        auto e = get_associated_executor(handler, self->get_executor());
        if (self->count())
        {
            self->decrement();
            BOOST_ASEM_ASIO_NAMESPACE::post(std::move(e),
                        BOOST_ASEM_ASIO_NAMESPACE::append(
                               std::forward< Handler >(handler), error_code()));
            return;
        }

        using handler_type = std::decay_t< Handler >;
        using model_type = detail::semaphore_wait_op_model< decltype(e), handler_type >;
        model_type *model = model_type ::construct(self, std::move(e), std::forward< Handler >(handler));

        self->add_waiter(model);
    }
};

template < class Executor >
template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionHandler >
BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionHandler, void(error_code))
basic_semaphore< Executor >::async_acquire(CompletionHandler &&token)
{
    return BOOST_ASEM_ASIO_NAMESPACE::async_initiate< CompletionHandler, void(std::error_code) >(
                async_aquire_op{this}, token);
}

BOOST_ASEM_END_NAMESPACE

#endif
