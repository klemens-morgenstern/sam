//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASEM_DETAIL_SERVICE_IPP
#define BOOST_ASEM_DETAIL_SERVICE_IPP

#include <boost/asem/detail/service.hpp>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

#if defined(BOOST_ASEM_SOURCE)
extern template struct op_list_service<st>;
extern template struct op_list_service<mt>;
extern template struct service_member<st>;
extern template struct service_member<mt>;
#endif

}
BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_DETAIL_SERVICE_IPP
