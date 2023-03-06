// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_DETAIL_MUTEX_IMPL_HPP
#define BOOST_ASEM_DETAIL_MUTEX_IMPL_HPP

#include <atomic>
#include <mutex>
#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/service.hpp>
#include <boost/asem/detail/basic_op_model.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{

struct mutex_impl : detail::service_member
{
    mutex_impl(net::execution_context & ctx) : detail::service_member(ctx)
    {
      if (thread_safe())
        new (&ts_locked_) std::atomic<bool>(false);
      else
        new (&locked_) bool(false);
    }

    BOOST_ASEM_DECL void unlock();
    bool try_lock()
    {
        if (thread_safe())
          return !ts_locked_.exchange(true);
        else
        {
          if (locked_)
            return false;
          else
            return locked_ = true;
        }
    }

    BOOST_ASEM_DECL void
    add_waiter(detail::wait_op *waiter) noexcept;
    BOOST_ASEM_DECL void lock(error_code & ec);

    void shutdown() override
    {
        auto w = std::move(waiters_);
        w.shutdown();
    }

    union {
      bool locked_;
      std::atomic<bool> ts_locked_;
    };

    detail::basic_bilist_holder<void(error_code)> waiters_;

    mutex_impl() = delete;
    mutex_impl(const mutex_impl &) = delete;
    mutex_impl(mutex_impl && mi)
        : detail::service_member(std::move(mi))
        , waiters_(std::move(mi.waiters_))
    {
      if (thread_safe())
        new (&ts_locked_) std::atomic<bool>(mi.ts_locked_.exchange(false));
      else
      {
        new (&locked_) bool(mi.locked_);
        mi.locked_ = false;
      }
    }

    mutex_impl& operator=(const mutex_impl & lhs) = delete;
    mutex_impl& operator=(mutex_impl && lhs) noexcept
    {
        auto _ = lhs.internal_lock();
        if (thread_safe())
          new (&ts_locked_) std::atomic<bool>(lhs.ts_locked_.exchange(false));
        else
        {
          new (&locked_) bool(lhs.locked_);
          lhs.locked_ = false;
        }
        lhs.waiters_ = std::move(waiters_);
        return *this;
    }
};

}

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_DETAIL_MUTEX_IMPL_HPP
