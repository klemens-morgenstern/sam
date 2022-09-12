//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_DETAIL_PREDICATE_OP
#define BOOST_ASEM_DETAIL_PREDICATE_OP

#include <boost/asem/detail/bilist_node.hpp>
#include <boost/asem/detail/config.hpp>

BOOST_ASEM_BEGIN_NAMESPACE


namespace detail
{
template<typename Signature>
struct predicate_op;

template<typename ... Ts>
struct predicate_op<void(Ts...)> : detail::bilist_node
{
    virtual void complete(Ts...) = 0;
    virtual bool done() = 0;
};

using predicate_wait_op = predicate_op<void(error_code)>;


template<typename Signature>
struct predicate_bilist_holder;

template<typename ...Ts>
struct predicate_bilist_holder<void(error_code, Ts...)> : bilist_node
{
    ~predicate_bilist_holder()
    {
        using op = predicate_op<void(error_code, Ts...)>;
        auto & nx = this->next_;
        while (nx != this)
            static_cast< op * >(nx)->complete(asio::error::operation_aborted, Ts{}...);
    }
};

}   // namespace detail
BOOST_ASEM_END_NAMESPACE

#endif