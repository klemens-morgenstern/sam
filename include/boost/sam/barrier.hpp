// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_BARRIER_HPP
#define BOOST_SAM_BARRIER_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_barrier.hpp>

BOOST_SAM_BEGIN_NAMESPACE

/// basic_barrier with default executor.
using barrier = basic_barrier<>;

BOOST_SAM_END_NAMESPACE

#endif //BOOST_SAM_BARRIER_HPP
