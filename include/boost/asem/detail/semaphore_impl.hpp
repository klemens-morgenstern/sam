// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_DETAIL_SEMAPHORE_IMPL_HPP
#define BOOST_ASEM_DETAIL_SEMAPHORE_IMPL_HPP

#include <mutex>
#include <boost/asem/detail/basic_op_model.hpp>
#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/service.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

struct semaphore_impl : detail::service_member
{
    BOOST_ASEM_DECL semaphore_impl(net::execution_context & ctx, int initial_count = 1);

    semaphore_impl(const semaphore_impl &) = delete;
    semaphore_impl(semaphore_impl && mi)
        : detail::service_member(std::move(mi))
        , count_(mi.count_)
        , waiters_(std::move(mi.waiters_))
    {
    }

    semaphore_impl& operator=(const semaphore_impl &) = delete;
    semaphore_impl& operator=(semaphore_impl && lhs) noexcept
    {
      auto _ = internal_lock();
      count_ = lhs.count_;
      std::swap(lhs.waiters_, waiters_);
      return *this;
    }

    void shutdown() override
    {
        auto w = std::move(waiters_);
        w.shutdown();
    }

    BOOST_ASEM_DECL bool
    try_acquire();

    BOOST_ASEM_DECL void acquire(error_code & ec);

    BOOST_ASEM_DECL void
    release();

    BOOST_ASEM_NODISCARD BOOST_ASEM_DECL int
    value() const noexcept;

    BOOST_ASEM_DECL void
    add_waiter(detail::wait_op *waiter) noexcept;

    BOOST_ASEM_DECL int
    decrement();

    BOOST_ASEM_NODISCARD BOOST_ASEM_DECL int
    count() const noexcept;


  private:
    int count_;
    detail::basic_bilist_holder<void(error_code)> waiters_;
};


}

BOOST_ASEM_END_NAMESPACE




#endif //BOOST_ASEM_DETAIL_SEMAPHORE_IMPL_HPP
