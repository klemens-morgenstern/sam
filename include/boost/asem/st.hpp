// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_ST_HPP
#define BOOST_ASEM_ST_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_mutex.hpp>
#include <boost/asem/basic_semaphore.hpp>
#include <boost/asem/st/basic_mutex.hpp>
#include <boost/asem/st/basic_semaphore.hpp>
#include <boost/asem/st/basic_condition_variable.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

/// The single-threaded versions of the primitives.
struct st
{
    /// The single threaded of a basic_semaphore. Use rebind_executor to change the executor.
    using semaphore = basic_semaphore<st>;
    /// The single threaded of a basic_mutex. Use rebind_executor to change the executor.
    using mutex = basic_mutex<st>;
    /// The single threaded of a basic_condition_variable. Use rebind_executor to change the executor.
    using condition_variable = basic_condition_variable<st>;
};


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_semaphore<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
extern template
struct basic_mutex<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
extern template
struct basic_condition_variable<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_ST_HPP
