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
struct mutex_impl<mt>
{
    std::atomic<bool> locked_ = false;
    bool locked() const {return locked_;}
    void do_lock()   { locked_ = true; }

    BOOST_ASEM_DECL void unlock();
    bool try_lock()
    {
        return locked_.exchange(true);
    }

    BOOST_ASEM_DECL void
    add_waiter(detail::wait_op *waiter) noexcept;

    auto lock() -> std::lock_guard<std::mutex>
    {
        return std::lock_guard<std::mutex>{mtx_};
    }

    std::mutex mtx_;
    detail::basic_bilist_holder<void(error_code)> waiters_;
};

}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_mutex<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/mt/impl/basic_mutex.ipp>
#endif


#endif //BOOST_ASEM_MT_BASIC_MUTEX_HPP
