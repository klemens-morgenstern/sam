// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_MUTEX_IMPL_HPP
#define BOOST_SAM_DETAIL_MUTEX_IMPL_HPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/service.hpp>
#include <boost/sam/detail/basic_op_model.hpp>

#include <mutex>

BOOST_SAM_BEGIN_NAMESPACE

namespace detail
{

struct mutex_impl : detail::service_member
{
    mutex_impl(net::execution_context & ctx) : detail::service_member(ctx), locked_(false)
    {
    }

    BOOST_SAM_DECL void unlock();
    bool try_lock()
    {
      auto _ = internal_lock();
      if (locked_)
        return false;
      else
        return locked_ = true;
    }

    BOOST_SAM_DECL void
    add_waiter(detail::wait_op *waiter) noexcept;
    BOOST_SAM_DECL void lock(error_code & ec);

    void shutdown() override
    {
        auto w = std::move(waiters_);
        w.shutdown();
    }

    bool locked_;

    detail::basic_bilist_holder<void(error_code)> waiters_;

    mutex_impl() = delete;
    mutex_impl(const mutex_impl &) = delete;
    mutex_impl(mutex_impl && mi)
        : detail::service_member(std::move(mi))
        , locked_(mi.locked_)
        , waiters_(std::move(mi.waiters_))
    {
      mi.locked_ = false;
    }

    mutex_impl& operator=(const mutex_impl & lhs) = delete;
    mutex_impl& operator=(mutex_impl && lhs) noexcept
    {
        auto _ = lhs.internal_lock();
        locked_ = lhs.locked_;
          lhs.locked_ = false;
        lhs.waiters_ = std::move(waiters_);
        return *this;
    }
};

}

BOOST_SAM_END_NAMESPACE

#endif //BOOST_SAM_DETAIL_MUTEX_IMPL_HPP
