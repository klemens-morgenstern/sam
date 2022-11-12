// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_IMPL_BASIC_CONDITION_VARIABLE_HPP
#define BOOST_ASEM_IMPL_BASIC_CONDITION_VARIABLE_HPP

#include <boost/asem/basic_condition_variable.hpp>
#include <boost/asem/detail/predicate_op_model.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

template < class Implementation, class Executor >
template < class Predicate >
struct basic_condition_variable<Implementation, Executor>::async_predicate_wait_op
{
    basic_condition_variable<Implementation, Executor> * self;
    Predicate predicate;
    template< class Handler >
    void operator()(Handler &&handler)
    {
        auto e = get_associated_executor(handler, self->get_executor());
        auto l = self->impl_.lock();
        ignore_unused(l);

        if (predicate())
            return net::post(
                    std::move(e),
                    net::append(
                            std::forward< Handler >(handler), error_code()));

        using handler_type = std::decay_t< Handler >;
        using predicate_type = Predicate;
        using model_type = detail::predicate_op_model< Implementation, decltype(e), handler_type,
                                                       predicate_type, void(error_code)>;
        model_type *model = model_type::construct(std::move(e),
                                                  std::forward< Handler >(handler),
                                                  std::move(predicate));
        self->impl_.add_waiter(model);
    }
};

template < class Implementation, class Executor >
struct basic_condition_variable<Implementation, Executor>::async_wait_op
{
    basic_condition_variable<Implementation, Executor> * self;

    struct true_predicate
    {
        constexpr bool operator()() const noexcept {return true;}
    };


    template< class Handler >
    void operator()(Handler &&handler)
    {
        auto e = get_associated_executor(handler, self->get_executor());
        auto l = self->impl_.lock();
        ignore_unused(l);

        using handler_type = std::decay_t< Handler >;
        using predicate_type = true_predicate;
        using model_type = detail::predicate_op_model< Implementation, decltype(e), handler_type,
                predicate_type, void(error_code)>;
        model_type *model = model_type::construct(std::move(e),
                                                  std::forward< Handler >(handler),
                                                  true_predicate{});
        self->impl_.add_waiter(model);
    }
};

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_IMPL_BASIC_CONDITION_VARIABLE_HPP
