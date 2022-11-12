// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_BASIC_MUTEX_HPP
#define BOOST_ASEM_MT_BASIC_MUTEX_HPP

#include <atomic>
#include <mutex>
#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_mutex.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

template<>
struct mutex_impl<mt> : detail::service_member<mt>
{
    mutex_impl(net::execution_context & ctx) : detail::service_member<mt>(ctx) {}

    bool locked() const {return locked_;}
    void do_lock()   { locked_ = true; }

    BOOST_ASEM_DECL void unlock();
    bool try_lock()
    {
        return !locked_.exchange(true);
    }

    BOOST_ASEM_DECL void
    add_waiter(detail::wait_op *waiter) noexcept;
    BOOST_ASEM_DECL void lock(error_code & ec);

    auto internal_lock() -> std::unique_lock<std::mutex>
    {
        return std::unique_lock<std::mutex>{mtx_};
    }

    void shutdown() override
    {
        auto w = std::move(waiters_);
        w.shutdown();
    }

    std::atomic<bool> locked_{false};
    std::mutex mtx_;
    detail::basic_bilist_holder<void(error_code)> waiters_;

    mutex_impl() = delete;
    mutex_impl(const mutex_impl &) = delete;
    mutex_impl(mutex_impl && mi)
        : detail::service_member<mt>(std::move(mi))
        , locked_(mi.locked_.exchange(false)), waiters_(std::move(mi.waiters_)) {}

    mutex_impl& operator=(const mutex_impl & lhs) = delete;
    mutex_impl& operator=(mutex_impl && lhs) noexcept
    {
        std::lock_guard<std::mutex> _(mtx_);

        lhs.locked_ = locked_.exchange(lhs.locked_.load());
        lhs.waiters_ = std::move(waiters_);
        return *this;
    }
};

}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_mutex<mt, net::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/mt/impl/basic_mutex.ipp>
#endif


#endif //BOOST_ASEM_MT_BASIC_MUTEX_HPP
