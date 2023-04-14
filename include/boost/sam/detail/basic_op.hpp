//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_SAM_DETAIL_SEMAPHORE_WAIT_OP
#define BOOST_SAM_DETAIL_SEMAPHORE_WAIT_OP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/bilist_node.hpp>
#include <boost/sam/detail/conditionally_enabled_mutex.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/cancellation_type.hpp>
#else
#include <boost/asio/cancellation_type.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{
struct basic_op : detail::bilist_node
{
  virtual void shutdown()      = 0;
  virtual void complete(error_code ec) = 0;
};


struct basic_bilist_holder : bilist_node
{
  ~basic_bilist_holder()
  {
    using op      = basic_op;
    auto      &nx = this->next_;
    error_code ec = asio::error::operation_aborted;
    while (nx != this)
      static_cast<op *>(nx)->complete(ec);
  }

  void complete_all(error_code ec)
  {
    using op = basic_op;
    auto &nx = this->next_;
    while (nx != this)
      static_cast<op *>(nx)->complete(ec);
  }

  void shutdown()
  {
    using op = basic_op;
    bilist_node bn{std::move(*this)};

    auto &nx = bn.next_;
    while (nx != &bn)
    {
      auto nx2 = nx->next_;
      static_cast<op *>(nx)->shutdown();
      nx = nx2;
    }
  }

  basic_bilist_holder() noexcept                              = default;
  basic_bilist_holder(basic_bilist_holder &&)                 = default;
  basic_bilist_holder(basic_bilist_holder const &)            = delete;
  basic_bilist_holder &operator=(basic_bilist_holder &&)      = default;
  basic_bilist_holder &operator=(basic_bilist_holder const &) = delete;
};


struct cancel_handler
{
  basic_op * model;
  detail::conditionally_enabled_mutex &mtx;

  cancel_handler(detail::basic_op * model,
                 detail::conditionally_enabled_mutex &mutex) : model(model), mtx(mutex) {}

  void operator()(net::cancellation_type type) const
  {
    if (type != net::cancellation_type::none)
    {
      detail::conditionally_enabled_mutex::scoped_lock lock{mtx};
      auto *self = model;
      self->complete(net::error::operation_aborted);
    }
  }
};

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif