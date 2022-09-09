// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_IMPL_ST_IPP
#define BOOST_ASEM_IMPL_ST_IPP

#include <boost/asem/st.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

#if defined(BOOST_ASEM_SOURCE)
template struct basic_semaphore<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
template struct basic_mutex<st, BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >;
#endif

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_IMPL_ST_IPP
