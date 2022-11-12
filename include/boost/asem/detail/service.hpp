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
#include <mutex>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/detail/null_mutex.hpp>
#include <asio/execution_context.hpp>
#else
#include <boost/asio/detail/null_mutex.hpp>
#include <boost/asio/execution_context.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

struct mt;
struct st;

namespace detail
{

template<typename Impl>
struct service_member;

template<typename Impl>
using mutex = typename std::conditional<
    std::is_same<st, Impl>::value,
    BOOST_ASEM_ASIO_NAMESPACE::detail::null_mutex,
    std::mutex>::type;

// Default service implementation for a strand.
template<typename Impl>
struct op_list_service final
  : BOOST_ASEM_ASIO_NAMESPACE::detail::execution_context_service_base<op_list_service<Impl>>
{
  op_list_service(asio::execution_context& ctx)
    : BOOST_ASEM_ASIO_NAMESPACE::detail::execution_context_service_base<op_list_service<Impl>>(ctx)
  {
  }

  bilist_node entries;
  using mutex_type = mutex<Impl>;
  mutex_type mtx_;

  void   register_queue(bilist_node * sm)
  {
    std::lock_guard<mutex_type> lock{mtx_};
    entries.link_before(sm);
  }
  void unregister_queue(bilist_node * sm)
  {
    std::lock_guard<mutex_type> lock{mtx_};
    sm->unlink();
  }

  void shutdown() override
  {
    using op = service_member<Impl>;
    auto e = std::move(entries);
    auto nx = e.next_;
    while (nx != &e)
    {
      auto nnx = nx->next_;
      static_cast< op * >(nx)->shutdown();
      nx = nnx;
    }

  }

  ~op_list_service()
  {
  }
};


template<typename Impl>
struct service_member : bilist_node
{

  op_list_service<Impl> * service;

  service_member(BOOST_ASEM_ASIO_NAMESPACE::execution_context & ctx)
    : service(&BOOST_ASEM_ASIO_NAMESPACE::use_service<op_list_service<Impl>>(ctx))
  {
    service->register_queue(this);
  }

  service_member(const service_member& ) = delete;
  service_member(service_member&& sm) noexcept
      : service(sm.service)
  {

  }
  service_member& operator=(const service_member& ) = delete;

  service_member& operator=(service_member&& sm) noexcept
  {
    if (sm.service != service)
    {
      sm.service->unregister_queue(this);
      sm.service = service;
      sm.service->register_queue(this);
    }
    return *this;
  }

  ~service_member()
  {
    service->unregister_queue(this);
  }

  using mutex_type = mutex<Impl>;

  virtual void shutdown() = 0;

  mutex_type mtx_;
  auto internal_lock() -> std::unique_lock<mutex_type>
  {
    return std::unique_lock<mutex_type>{mtx_};
  }
};

#if !defined(BOOST_ASEM_HEADER_ONLY)
extern template struct op_list_service<st>;
extern template struct op_list_service<mt>;
extern template struct service_member<st>;
extern template struct service_member<mt>;
#endif

}
BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_DETAIL_SERVICE_HPP
