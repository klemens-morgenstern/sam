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
#include <boost/asem/detail/basic_op_model.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/post.hpp>
#else
#include <boost/asio/post.hpp>
#endif


BOOST_ASEM_BEGIN_NAMESPACE

template < class Implementation, class Executor >
basic_semaphore<  Implementation, Executor >::basic_semaphore(executor_type exec,
                                                         int initial_count)
: impl_(initial_count)
, exec_(std::move(exec))
{
}

template < class  Implementation, class Executor >
auto
basic_semaphore< Implementation, Executor >::get_executor() const -> executor_type const &
{
    return exec_;
}


template < class  Implementation, class Executor >
struct basic_semaphore<Implementation ,Executor>::async_aquire_op
{
    basic_semaphore<Implementation, Executor> * self;

    template< class Handler >
    void operator()(Handler &&handler)
    {
        auto e = get_associated_executor(handler, self->get_executor());
        auto l = self->impl_.lock();
        ignore_unused(l);
        if (self->impl_.count() != 0)
        {
            self->impl_.decrement();
            BOOST_ASEM_ASIO_NAMESPACE::post(std::move(e),
                                            BOOST_ASEM_ASIO_NAMESPACE::append(
                                                std::forward< Handler >(handler), error_code()));
        }

        using handler_type = std::decay_t< Handler >;
        using model_type = detail::basic_op_model< Implementation, decltype(e), handler_type, void(error_code)>;
        model_type *model = model_type ::construct(std::move(e), std::forward< Handler >(handler));
        self->impl_.add_waiter(model);
    }
};

template < class  Implementation, class Executor >
template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionHandler >
BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionHandler, void(error_code))
basic_semaphore<Implementation, Executor >::async_acquire(CompletionHandler &&token)
{
    return BOOST_ASEM_ASIO_NAMESPACE::async_initiate< CompletionHandler, void(std::error_code) >(
                async_aquire_op{this}, token);
}

BOOST_ASEM_END_NAMESPACE

#endif
