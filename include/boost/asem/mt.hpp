// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_HPP
#define BOOST_ASEM_MT_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_semaphore.hpp>
#include <boost/asem/basic_mutex.hpp>
#include <boost/asem/basic_barrier.hpp>
#include <boost/asem/mt/basic_mutex.hpp>
#include <boost/asem/mt/basic_semaphore.hpp>
#include <boost/asem/mt/basic_condition_variable.hpp>
#include <boost/asem/mt/basic_barrier.hpp>

BOOST_ASEM_BEGIN_NAMESPACE


/// The multi threaded, i.e. thread-safe scope for the primitives.
struct mt
{
    /// The multi threaded version of a basic_semaphore. Use rebind_executor to change the executor.
    using semaphore = basic_semaphore<mt>;
    /// The multi threaded version of a basic_mutex. Use rebind_executor to change the executor.
    using mutex = basic_mutex<mt>;
    /// The multi threaded version of a basic_condition_variable. Use rebind_executor to change the executor.
    using condition_variable = basic_condition_variable<mt>;
    /// The multi threader version of the barrier
    using barrier = basic_barrier<mt>;
};


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_semaphore<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
extern template
struct basic_mutex<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
extern template
struct basic_condition_variable<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
extern template
struct barrier<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_MT_HPP
