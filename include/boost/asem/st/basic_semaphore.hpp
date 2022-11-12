// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_BASIC_SEMAPHORE_HPP
#define BOOST_ASEM_ST_BASIC_SEMAPHORE_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_semaphore.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

template<>
struct semaphore_impl<st> : detail::service_member<st>
{
    BOOST_ASEM_DECL semaphore_impl(BOOST_ASEM_ASIO_NAMESPACE::execution_context & ctx, int initial_count = 1);

    semaphore_impl(const semaphore_impl &) = delete;
    semaphore_impl(semaphore_impl && mi)
        : detail::service_member<st>(std::move(mi))
        , count_(mi.count_), waiters_(std::move(mi.waiters_))
    {
        mi.count_= 1;
    }

    semaphore_impl& operator=(const semaphore_impl &) = delete;
    semaphore_impl& operator=(semaphore_impl && lhs) noexcept
    {
        std::swap(lhs.count_, count_);
        std::swap(lhs.waiters_, waiters_);
        return *this;
    }

    void shutdown() override
    {
        auto w = std::move(waiters_);
        w.shutdown();
    }

    BOOST_ASEM_DECL bool
    try_acquire();

    BOOST_ASEM_DECL void acquire(error_code & ec)
    {
        if (!try_acquire())
            ec = asio::error::in_progress;
    }

    BOOST_ASEM_DECL void
    release();

    BOOST_ASEM_NODISCARD BOOST_ASEM_DECL int
    value() const noexcept;

    BOOST_ASEM_DECL void
    add_waiter(detail::wait_op *waiter) noexcept;

    BOOST_ASEM_DECL int
    decrement();

    BOOST_ASEM_NODISCARD BOOST_ASEM_DECL int
    count() const noexcept;

    std::nullptr_t internal_lock() {return nullptr;}


  private:
    int count_;
    detail::basic_bilist_holder<void(error_code)> waiters_;
};


}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_semaphore<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/st/impl/basic_semaphore.ipp>
#endif


#endif //BOOST_ASEM_ST_BASIC_SEMAPHORE_HPP
