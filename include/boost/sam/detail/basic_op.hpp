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

#include <boost/sam/detail/bilist_node.hpp>
#include <boost/sam/detail/config.hpp>

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{
template <typename Signature>
struct basic_op;

template <typename... Ts>
struct basic_op<void(Ts...)> : detail::bilist_node
{
  virtual void shutdown()      = 0;
  virtual void complete(Ts...) = 0;
};

using wait_op = basic_op<void(error_code)>;

template <typename Signature>
struct basic_bilist_holder;

template <typename... Ts>
struct basic_bilist_holder<void(error_code, Ts...)> : bilist_node
{
  ~basic_bilist_holder()
  {
    using op      = basic_op<void(error_code, Ts...)>;
    auto      &nx = this->next_;
    error_code ec = asio::error::operation_aborted;
    while (nx != this)
      static_cast<op *>(nx)->complete(ec, Ts{}...);
  }

  void complete_all(error_code ec, Ts... ts)
  {
    using op = basic_op<void(error_code, Ts...)>;
    auto &nx = this->next_;
    while (nx != this)
      static_cast<op *>(nx)->complete(ec, std::move(ts)...);
  }

  void shutdown()
  {
    using op = basic_op<void(error_code, Ts...)>;
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

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif