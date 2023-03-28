// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_CONDITION_VARIABLE_IMPL_HPP
#define BOOST_SAM_DETAIL_CONDITION_VARIABLE_IMPL_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/predicate_op.hpp>
#include <boost/sam/detail/service.hpp>
#include <mutex>

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct condition_variable_impl : detail::service_member
{
  BOOST_SAM_DECL condition_variable_impl(net::execution_context &ctx,
                                         int concurrency_hint = BOOST_ASIO_CONCURRENCY_HINT_DEFAULT);

  condition_variable_impl(condition_variable_impl const &) = delete;
  condition_variable_impl(condition_variable_impl &&lhs) noexcept
      : detail::service_member(std::move(lhs)), waiters_(std::move(lhs.waiters_))
  {
  }

  condition_variable_impl &operator=(condition_variable_impl const &) = delete;

  condition_variable_impl &operator=(condition_variable_impl &&lhs) noexcept
  {
    detail::service_member::operator=(std::move(lhs));
    std::swap(lhs.waiters_, waiters_);
    return *this;
  }

  void shutdown() override
  {
    lock_type l{mtx_};
    auto w = std::move(waiters_);
    l.unlock();
    w.shutdown();
  }

  BOOST_SAM_DECL void notify_one();
  BOOST_SAM_DECL void notify_all();

  BOOST_SAM_DECL void add_waiter(detail::predicate_wait_op *waiter) noexcept;

private:
  detail::predicate_bilist_holder<void(error_code)> waiters_;
};

} // namespace detail

BOOST_SAM_END_NAMESPACE

#if defined(BOOST_SAM_HEADER_ONLY)
#include <boost/sam/detail/impl/condition_variable_impl.ipp>
#endif

#endif // BOOST_SAM_DETAIL_CONDITION_VARIABLE_IMPL_HPP
