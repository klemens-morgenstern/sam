//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SAM_DETAIL_SERVICE_HPP
#define BOOST_SAM_DETAIL_SERVICE_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/bilist_node.hpp>
#include <boost/sam/detail/concurrency_hint.hpp>
#include <boost/sam/detail/conditionally_enabled_mutex.hpp>
#include <mutex>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/detail/null_mutex.hpp>
#include <asio/execution_context.hpp>
#else
#include <boost/asio/detail/null_mutex.hpp>
#include <boost/asio/execution_context.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct service_member;

// Default service implementation for a strand.
struct op_list_service final : net::detail::execution_context_service_base<op_list_service>
{
  BOOST_SAM_DECL explicit op_list_service(asio::execution_context &ctx);

  bilist_node             entries;

  using mutex_type = detail::conditionally_enabled_mutex;
  using lock_type  = typename mutex_type::scoped_lock;
  mutex_type mtx_;

  void register_queue(bilist_node *sm)
  {
    lock_type lock{mtx_};
    sm->link_before(&entries);
  }
  void unregister_queue(bilist_node *sm)
  {
    lock_type lock{mtx_};
    sm->unlink();
  }

  BOOST_SAM_DECL void shutdown() override;
  ~op_list_service() final = default;
};

struct service_member : bilist_node
{
  op_list_service *service;

  explicit service_member(net::execution_context &ctx,
                          int concurrency_hint = BOOST_SAM_CONCURRENCY_HINT_DEFAULT)
      : service(&net::use_service<op_list_service>(ctx)),
        mtx_(!detail::is_single_threaded(ctx, concurrency_hint))
  {
    service->register_queue(this);
  }

  service_member(const service_member &) = delete;
  service_member(service_member &&sm) noexcept : service(sm.service), mtx_(sm.mtx_.enabled())
  {
    service->register_queue(this);
  }
  service_member &operator=(const service_member &) = delete;

  service_member &operator=(service_member &&sm) noexcept
  {
    if (sm.service != service)
    {
      service->unregister_queue(this);
      sm.service = service;
      service->register_queue(this);
    }
    return *this;
  }

  ~service_member()
  {
    lock_type _{mtx_};;
    if (service != nullptr)
      service->unregister_queue(this);
  }

  using mutex_type        = detail::conditionally_enabled_mutex;
  using lock_type         = typename mutex_type::scoped_lock;
  virtual void shutdown() = 0;

  mutable mutex_type mtx_;
};

} // namespace detail
BOOST_SAM_END_NAMESPACE

#if defined(BOOST_SAM_HEADER_ONLY)
#include <boost/sam/detail/impl/service.ipp>
#endif

#endif // BOOST_SAM_DETAIL_SERVICE_HPP
