// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_IMPL_MT_IPP
#define BOOST_ASEM_IMPL_MT_IPP

#include <boost/asem/mt.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

#if defined(BOOST_ASEM_SOURCE)
template struct basic_barrier<mt, net::any_io_executor >;
template struct basic_condition_variable<mt, net::any_io_executor >;
template struct basic_mutex<mt, net::any_io_executor >;
template struct basic_semaphore<mt, net::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_IMPL_MT_IPP
