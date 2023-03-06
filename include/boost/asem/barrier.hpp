// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_BARRIER_HPP
#define BOOST_ASEM_BARRIER_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/basic_barrier.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

/// basic_barrier with default executor.
using barrier = basic_barrier<>;

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_BARRIER_HPP
