// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_BASIC_BARRIER_HPP
#define BOOST_ASEM_ST_BASIC_BARRIER_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_barrier.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

template<>
struct barrier_impl<st>
{
    std::ptrdiff_t init_;
    std::ptrdiff_t counter_ = init_;

    BOOST_ASEM_DECL bool try_arrive();
    BOOST_ASEM_DECL void add_waiter(detail::wait_op *waiter) noexcept;

    std::nullptr_t lock()
    {
        return nullptr;
    }

    detail::basic_bilist_holder<void(error_code)> waiters_;
};

}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_barrier<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/st/impl/basic_barrier.ipp>
#endif


#endif //BOOST_ASEM_ST_BASIC_BARRIER_HPP