// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_SEMAPHORE_IMPL_HPP
#define BOOST_SAM_DETAIL_SEMAPHORE_IMPL_HPP

#include <boost/sam/detail/basic_op_model.hpp>
#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/service.hpp>
#include <mutex>

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct semaphore_impl : detail::service_member
{
  BOOST_SAM_DECL semaphore_impl(net::execution_context &ctx,
                                int initial_count = 1,
                                int concurrency_hint = BOOST_SAM_CONCURRENCY_HINT_DEFAULT);

  semaphore_impl(const semaphore_impl &) = delete;
  semaphore_impl(semaphore_impl &&mi)
      : detail::service_member(std::move(mi)), count_(mi.count_), waiters_(std::move(mi.waiters_))
  {
  }

  semaphore_impl &operator=(const semaphore_impl &) = delete;
  semaphore_impl &operator=(semaphore_impl &&lhs)
  {
    lock_type _{mtx_};
    count_ = lhs.count_;
    std::swap(lhs.waiters_, waiters_);
    return *this;
  }

  void shutdown() override
  {
    lock_type l{mtx_};;
    auto w = std::move(waiters_);
    l.unlock();
    w.shutdown();
  }

  BOOST_SAM_DECL bool try_acquire();

  BOOST_SAM_DECL void acquire(error_code &ec);

  BOOST_SAM_DECL void release();

  BOOST_SAM_NODISCARD BOOST_SAM_DECL int value() const;

  BOOST_SAM_DECL void add_waiter(detail::basic_op *waiter) noexcept;

  BOOST_SAM_DECL int decrement();

  BOOST_SAM_NODISCARD BOOST_SAM_DECL int count() const noexcept;

private:
  int                                           count_;
  detail::basic_bilist_holder waiters_;
  struct acquire_op_t;
};

} // namespace detail

BOOST_SAM_END_NAMESPACE

#if defined(BOOST_SAM_HEADER_ONLY)
#include <boost/sam/detail/impl/semaphore_impl.ipp>
#endif


#endif // BOOST_SAM_DETAIL_SEMAPHORE_IMPL_HPP
