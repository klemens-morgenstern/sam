//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_SHARED_LOCK_HPP
#define BOOST_SAM_SHARED_LOCK_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_shared_lock.hpp>
#include <mutex>
#include <utility>


BOOST_SAM_BEGIN_NAMESPACE

using shared_lock = basic_shared_lock<>;

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_SHARED_LOCK_HPP
