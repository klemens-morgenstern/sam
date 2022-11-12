// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_BASIC_CONDITION_VARIABLE_HPP
#define BOOST_ASEM_ST_BASIC_CONDITION_VARIABLE_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_condition_variable.hpp>
#include <boost/asem/detail/predicate_op.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

template<>
struct condition_variable_impl<st> : detail::service_member<st>
{
    BOOST_ASEM_DECL condition_variable_impl(net::execution_context & ctx);

    condition_variable_impl(condition_variable_impl const &) = delete;
    condition_variable_impl(condition_variable_impl && lhs) noexcept
        : detail::service_member<st>(std::move(lhs)), waiters_(std::move(lhs.waiters_)) {}

    condition_variable_impl & operator=(condition_variable_impl const &) = delete;
    condition_variable_impl & operator=(condition_variable_impl && lhs) noexcept
    {
        detail::service_member<st>::operator=(std::move(lhs));
        std::swap(lhs.waiters_, waiters_);
        return *this;
    }

    void shutdown() override
    {
      auto w = std::move(waiters_);
      w.shutdown();
    }

    BOOST_ASEM_DECL void
    notify_one();

    BOOST_ASEM_DECL void
    notify_all();

    BOOST_ASEM_DECL void
    add_waiter(detail::predicate_wait_op *waiter) noexcept;

    std::nullptr_t lock() {return nullptr;}

  private:
    detail::predicate_bilist_holder<void(error_code)> waiters_;
};


}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_condition_variable<st, net::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/st/impl/basic_condition_variable.ipp>
#endif


#endif //BOOST_ASEM_ST_BASIC_CONDITION_VARIABLE_HPP
