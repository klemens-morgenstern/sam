// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_SRC_HPP
#define BOOST_SAM_SRC_HPP

#define BOOST_SAM_SOURCE

#include <boost/sam/detail/config.hpp>

#if defined(BOOST_SAM_HEADER_ONLY)
#error Do not compile SaM library source with BOOST_BEAST_HEADER_ONLY defined
#endif

#include <boost/sam/detail/impl/barrier_impl.ipp>
#include <boost/sam/detail/impl/condition_variable_impl.ipp>
#include <boost/sam/detail/impl/mutex_impl.ipp>
#include <boost/sam/detail/impl/semaphore_impl.ipp>
#include <boost/sam/detail/impl/service.ipp>

#endif // BOOST_SAM_SRC_HPP
