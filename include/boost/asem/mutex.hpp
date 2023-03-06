// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MUTEX_HPP
#define BOOST_ASEM_MUTEX_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_mutex.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

/// basic_mutex with default executor.
using mutex = basic_mutex<>;

BOOST_ASEM_END_NAMESPACE
#endif //BOOST_ASEM_MUTEX_HPP
