// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_SEMAPHORE_HPP
#define BOOST_SAM_SEMAPHORE_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_semaphore.hpp>

BOOST_SAM_BEGIN_NAMESPACE

/// basic_semaphore with default executor.
using semaphore = basic_semaphore<>;

BOOST_SAM_END_NAMESPACE
#endif // BOOST_SAM_SEMAPHORE_HPP
