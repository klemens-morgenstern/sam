// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_BASIC_MUTEX_HPP
#define BOOST_ASEM_ST_BASIC_MUTEX_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_mutex.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

template<>
struct mutex_impl<st> : detail::service_member<st>
{
    mutex_impl(BOOST_ASEM_ASIO_NAMESPACE::execution_context & ctx) : detail::service_member<st>(ctx) {}

    bool locked_ = false;
    bool locked() const {return locked_;}
    void do_lock()   { locked_ = true; }
    BOOST_ASEM_DECL void unlock();
    BOOST_ASEM_DECL bool try_lock();
    BOOST_ASEM_DECL void add_waiter(detail::wait_op *waiter) noexcept;
    void lock(error_code & ec)
    {
        if (!try_lock())
            ec = asio::error::in_progress;
    }

    std::nullptr_t internal_lock()
    {
        return nullptr;
    }
    int i = 0;
    void shutdown() override
    {
        auto w = std::move(waiters_);
        w.shutdown();
    }

    detail::basic_bilist_holder<void(error_code)> waiters_;

    mutex_impl() = delete;
    mutex_impl(const mutex_impl &) = delete;
    mutex_impl(mutex_impl && mi)
        : detail::service_member<st>(std::move(mi))
        , locked_(mi.locked_), waiters_(std::move(mi.waiters_))
    {
        mi.locked_ = false;
    }

    mutex_impl& operator=(const mutex_impl &) = delete;
    mutex_impl& operator=(mutex_impl && lhs) noexcept
    {
        std::swap(lhs.locked_, locked_);
        std::swap(lhs.waiters_, waiters_);
        return *this;
    }

};

}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_mutex<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/st/impl/basic_mutex.ipp>
#endif


#endif //BOOST_ASEM_ST_BASIC_MUTEX_HPP
