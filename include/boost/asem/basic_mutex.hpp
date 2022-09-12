// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_BASIC_MUTEX_HPP
#define BOOST_ASEM_BASIC_MUTEX_HPP

#include <boost/asem/detail/config.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/any_io_executor.hpp>
#include <asio/compose.hpp>
#else
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/compose.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

struct st;
struct mt;

namespace detail
{

template<typename Impl>
struct mutex_impl;

}

template<typename Implementation, typename Executor = BOOST_ASEM_ASIO_NAMESPACE::any_io_executor>
struct basic_mutex
{
    using executor_type = Executor;

    explicit basic_mutex(executor_type exec)
            : exec_(std::move(exec))
    {
    }

    template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionToken
        BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
        BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
    async_lock(CompletionToken &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return BOOST_ASEM_ASIO_NAMESPACE::async_initiate<CompletionToken, void(std::error_code)>(
                async_lock_op{this}, token);
    }

    void
    unlock()
    {
        impl_.unlock();
    }

    bool
    try_lock()
    {
        return impl_.try_lock();
    }

    /// Rebinds the mutex type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The mutex type when rebound to the specified executor.
        typedef basic_mutex<Implementation, Executor1> other;
    };

    /// @brief return the default executor.
    executor_type
    get_executor() const noexcept {return exec_;}

  private:
    detail::mutex_impl<Implementation> impl_;
    Executor exec_;
    struct async_lock_op;
};


BOOST_ASEM_END_NAMESPACE

#include <boost/asem/impl/basic_mutex.hpp>

#endif //BOOST_ASEM_BASIC_MUTEX_HPP
