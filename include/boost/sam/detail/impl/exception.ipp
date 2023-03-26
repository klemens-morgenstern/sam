// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_IMPL_EXCEPTION_IPP
#define BOOST_SAM_DETAIL_IMPL_EXCEPTION_IPP

#include <boost/sam/detail/exception.hpp>

#if !defined(BOOST_SAM_STANDALONE)
#include <boost/throw_exception.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

BOOST_SAM_DECL
void detail::throw_error(const error_code & ec,
                         const std::string & prefix)
{
#if defined(BOOST_SAM_STANDALONE)
  throw system_error(ec);
#else
  throw_exception(system_error(ec));
#endif
}

BOOST_SAM_END_NAMESPACE

#endif //BOOST_SAM_DETAIL_IMPL_EXCEPTION_IPP
