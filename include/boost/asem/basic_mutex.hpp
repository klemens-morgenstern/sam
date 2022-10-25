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

/** An asio based mutex modeled on `std::mutex`.
 *
 * @tparam Implementation The implementation, st or mt.
 * @tparam Executor The executor to use as default completion.
 */
template<typename Implementation, typename Executor = BOOST_ASEM_ASIO_NAMESPACE::any_io_executor>
struct basic_mutex
{
    /// The executor type.
    using executor_type = Executor;

    /// The destructor. @param exec The executor to be used by the mutex.
    explicit basic_mutex(executor_type exec)
            : exec_(std::move(exec))
    {
    }

    /// The destructor. @param ctx The execution context used by the mutex.
    template<typename ExecutionContext>
    explicit basic_mutex(ExecutionContext & ctx,
                         typename std::enable_if<
                                 std::is_convertible<
                                         ExecutionContext&,
                                         BOOST_ASEM_ASIO_NAMESPACE::execution_context&>::value
                                 >::type * = nullptr)
            : exec_(ctx.get_executor())
    {
    }

    /// @brief Rebind a mutex to a new executor - this cancels all outstanding operations.
    template<typename Executor_>
    basic_mutex(basic_mutex<Implementation, Executor_> && sem,
                std::enable_if_t<std::is_convertible<Executor_, executor_type>::value> * = nullptr)
            : exec_(sem.get_executor())
    {
    }

    /** Wait for the mutex to become lockable & lock it.
     *
     * @tparam CompletionToken The completion token type.
     * @param token The token for completion.
     * @return Deduced from the token.
     */
    template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionToken
        BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
        BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
    async_lock(CompletionToken &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return BOOST_ASEM_ASIO_NAMESPACE::async_initiate<CompletionToken, void(std::error_code)>(
                async_lock_op{this}, token);
    }

    /* Lock synchronously. This may fail depending on the implementation.
     *
     * If the implementation is `st` this will generate an error if the mutex
     * is already locked.
     *
     * If the implementation is `mt` this function will block until another thread releases
     * the lock. Note that this may lead to deadlocks.
     *
     * You should never use the synchronous functions from within an asio event-queue.
     *
     */
    void  lock(error_code & ec)
    {
        impl_.lock(ec);
    }

    /// Throwing @overload lock(error_code &);
    void lock()
    {
        error_code ec;
        lock(ec);
        if (ec)
            throw system_error(ec, "lock");
    }
    /// Unlock the mutex, and complete one pending lock if pending.
    void
    unlock()
    {
        impl_.unlock();
    }

    ///  Try to lock the mutex.
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
