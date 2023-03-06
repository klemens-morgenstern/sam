// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_SEMAPHORE_HPP
#define BOOST_ASEM_SEMAPHORE_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_semaphore.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

/// basic_semaphore with default executor.
using semaphore = basic_semaphore<>;

BOOST_ASEM_END_NAMESPACE
#endif //BOOST_ASEM_SEMAPHORE_HPP
