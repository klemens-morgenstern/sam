//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_SHARED_MUTEX_HPP
#define BOOST_SAM_SHARED_MUTEX_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_shared_mutex.hpp>

BOOST_SAM_BEGIN_NAMESPACE

/// basic_mutex with default executor.
using shared_mutex = basic_shared_mutex<>;

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_SHARED_MUTEX_HPP
