// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_BARRIER_IMPL_HPP
#define BOOST_SAM_DETAIL_BARRIER_IMPL_HPP

#include <boost/sam/basic_barrier.hpp>
#include <boost/sam/detail/basic_op.hpp>
#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/service.hpp>

#include <mutex>

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct barrier_impl : detail::service_member
{
  barrier_impl(net::execution_context &ctx, std::ptrdiff_t init,
               int concurrency_hint = BOOST_SAM_CONCURRENCY_HINT_DEFAULT)
      : detail::service_member(ctx, concurrency_hint), init_(init), counter_(init_)

  {
  }

  barrier_impl(barrier_impl &&rhs) noexcept
      : detail::service_member(std::move(rhs)), init_(rhs.init_), counter_(rhs.counter_),
        waiters_(std::move(rhs.waiters_))
  {
  }

  barrier_impl &operator=(barrier_impl &&rhs)
  {
    lock_type l{mtx_};

    detail::service_member::operator=(std::move(rhs));
    init_    = rhs.init_;
    waiters_ = std::move(rhs.waiters_);
    counter_ = rhs.counter_;
    return *this;
  }

  std::ptrdiff_t init_;
  std::ptrdiff_t counter_{init_};

  BOOST_SAM_DECL bool try_arrive();
  BOOST_SAM_DECL void add_waiter(detail::basic_op *waiter) noexcept;
  BOOST_SAM_DECL void arrive(error_code &ec);

  void decrement() { counter_--; }
  void shutdown() override
  {
    lock_type l{mtx_};
    auto w = std::move(waiters_);
    l.unlock();
    w.shutdown();
  }
  detail::basic_bilist_holder waiters_;

  struct arrive_op_t;
};

} // namespace detail

BOOST_SAM_END_NAMESPACE

#if defined(BOOST_SAM_HEADER_ONLY)
#include <boost/sam/detail/impl/barrier_impl.ipp>
#endif

#endif // BOOST_SAM_DETAIL_BARRIER_IMPL_HPP
