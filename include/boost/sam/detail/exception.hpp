// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_ERROR_HPP
#define BOOST_SAM_DETAIL_ERROR_HPP

#include <boost/sam/detail/config.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

BOOST_SAM_DECL
void throw_error(const error_code & ec,
                 const std::string & prefix);

}
BOOST_SAM_END_NAMESPACE

#if defined(BOOST_SAM_HEADER_ONLY)
#include <boost/sam/detail/impl/exception.ipp>
#endif

#endif //BOOST_SAM_DETAIL_ERROR_HPP
