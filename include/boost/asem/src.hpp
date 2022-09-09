// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_SRC_HPP
#define BOOST_ASEM_SRC_HPP

#define BOOST_ASEM_SOURCE

#include <boost/process/v2/detail/config.hpp>

#if defined(BOOST_ASEM_HEADER_ONLY)
# error Do not compile Asem library source with BOOST_BEAST_HEADER_ONLY defined
#endif

#include <boost/asem/mt/impl/basic_semaphore.ipp>
#include <boost/asem/st/impl/basic_semaphore.ipp>
#include <boost/asem/mt/impl/basic_mutex.ipp>
#include <boost/asem/st/impl/basic_mutex.ipp>
#include <boost/asem/impl/st.ipp>
#include <boost/asem/impl/mt.ipp>

#endif //BOOST_ASEM_SRC_HPP
