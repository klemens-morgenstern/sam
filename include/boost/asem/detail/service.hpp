//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASEM_DETAIL_SERVICE_HPP
#define BOOST_ASEM_DETAIL_SERVICE_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/bilist_node.hpp>
#include <boost/asem/detail/concurrency_hint.hpp>
#include <mutex>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/detail/null_mutex.hpp>
#include <asio/execution_context.hpp>
#else
#include <boost/asio/detail/null_mutex.hpp>
#include <boost/asio/execution_context.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

struct service_member;

// Default service implementation for a strand.
struct op_list_service final
  : net::detail::execution_context_service_base<op_list_service>
{

  BOOST_ASEM_DECL op_list_service(asio::execution_context& ctx);
  bilist_node entries;

  using mutex_type = detail::conditionally_enabled_mutex;
  using lock_type = typename mutex_type::scoped_lock;
  mutex_type mtx_;

  void   register_queue(bilist_node * sm)
  {
    lock_type lock{mtx_};
    sm->link_before(&entries);
  }
  void unregister_queue(bilist_node * sm)
  {
    lock_type lock{mtx_};
    sm->unlink();
  }

  BOOST_ASEM_DECL void shutdown() override;
  ~op_list_service()
  {
  }
};


struct service_member : bilist_node
{

  op_list_service* service;

  service_member(net::execution_context & ctx)
    : service(&net::use_service<op_list_service>(ctx)),
      mtx_(!detail::is_single_threaded(ctx))
  {
    service->register_queue(this);
  }

  service_member(const service_member& ) = delete;
  service_member(service_member&& sm) noexcept
      : service(sm.service), mtx_(sm.mtx_.enabled())
  {
    service->register_queue(this);
  }
  service_member& operator=(const service_member& ) = delete;

  service_member& operator=(service_member&& sm) noexcept
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
    if (service != nullptr)
      service->unregister_queue(this);
  }

  using mutex_type = detail::conditionally_enabled_mutex;
  using lock_type = typename mutex_type::scoped_lock;
  virtual void shutdown() = 0;

  mutable mutex_type mtx_;
  auto internal_lock() const -> lock_type
  {
    return lock_type{mtx_};
  }

  bool multi_threaded() const
  {
    return mtx_.enabled();
  }
};

}
BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_DETAIL_SERVICE_HPP
