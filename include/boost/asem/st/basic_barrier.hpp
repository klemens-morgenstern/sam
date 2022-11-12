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
struct barrier_impl<st> : detail::service_member<st>
{
    barrier_impl(BOOST_ASEM_ASIO_NAMESPACE::execution_context & ctx,
                 std::ptrdiff_t init) : detail::service_member<st>(ctx), init_(init) {}

    std::ptrdiff_t init_;
    std::ptrdiff_t counter_ = init_;

    BOOST_ASEM_DECL bool try_arrive();
    BOOST_ASEM_DECL void add_waiter(detail::wait_op *waiter) noexcept;

    void arrive(error_code & ec)
    {
        if (!try_arrive())
            ec = asio::error::in_progress;
    }

    void shutdown() override
    {
      auto w = std::move(waiters_);
      w.shutdown();
    }

    void decrement()
    {
        counter_--;
    }
    std::nullptr_t internal_lock()
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
