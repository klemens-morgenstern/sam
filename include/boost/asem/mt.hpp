// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_HPP
#define BOOST_ASEM_MT_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_semaphore.hpp>
#include <boost/asem/basic_mutex.hpp>
#include <boost/asem/mt/basic_mutex.hpp>
#include <boost/asem/mt/basic_semaphore.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

}

struct mt
{
    using semaphore = basic_semaphore<mt>;
    using mutex = basic_mutex<mt>;
};


#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template
struct basic_semaphore<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
extern template
struct basic_mutex<mt, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_MT_HPP
