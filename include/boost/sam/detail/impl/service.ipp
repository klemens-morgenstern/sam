//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_DETAIL_SERVICE_IPP
#define BOOST_SAM_DETAIL_SERVICE_IPP

#include <boost/sam/detail/service.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

op_list_service::op_list_service(asio::execution_context& ctx)
    : net::detail::execution_context_service_base<op_list_service>(ctx),
      mtx_(!detail::is_single_threaded(ctx))
{
}


void op_list_service::shutdown()
{
  using op = service_member;
  auto e = std::move(entries);
  auto nx = e.next_;
  while (nx != &e)
  {
    auto nnx = nx->next_;
    static_cast< op * >(nx)->service = nullptr;
    static_cast< op * >(nx)->shutdown();
    e.next_ = nx = nnx;
  }
}


}
BOOST_SAM_END_NAMESPACE

#endif //BOOST_SAM_DETAIL_SERVICE_IPP
