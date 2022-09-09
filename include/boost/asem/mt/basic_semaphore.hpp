// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_BASIC_SEMAPHORE_HPP
#define BOOST_ASEM_MT_BASIC_SEMAPHORE_HPP

#include <atomic>
#include <mutex>
#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_semaphore.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

template<>
struct semaphore_impl<mt>
{
    BOOST_ASEM_DECL semaphore_impl(int initial_count = 1);

    semaphore_impl(semaphore_impl const &) = delete;

    semaphore_impl &
    operator=(semaphore_impl const &) = delete;

    semaphore_impl(semaphore_impl &&) = delete;

    semaphore_impl &
    operator=(semaphore_impl &&) = delete;


    BOOST_ASEM_DECL bool
    try_acquire();

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

    BOOST_ASEM_DECL std::lock_guard<std::mutex> lock();

  private:
    detail::basic_bilist_holder<void(error_code)> waiters_;
    std::atomic<int> count_;
    mutable std::mutex mtx_;
};


}


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_semaphore<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE


#if defined(BOOST_ASEM_HEADER_ONLY)
#include <boost/asem/mt/impl/basic_semaphore.ipp>
#endif


#endif //BOOST_ASEM_MT_BASIC_SEMAPHORE_HPP
