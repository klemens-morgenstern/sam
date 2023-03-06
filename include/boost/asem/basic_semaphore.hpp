// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASEM_BASIC_SEMAPHORE_HPP
#define BOOST_ASEM_BASIC_SEMAPHORE_HPP


#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/bilist_node.hpp>
#include <boost/asem/detail/semaphore_impl.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/any_io_executor.hpp>
#include <asio/async_result.hpp>
#include <asio/detail/config.hpp>
#else
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

/** An asio based semaphore.`
 *
 * @tparam Implementation The implementation, st or mt.
 * @tparam Executor The executor to use as default completion.
 */
template < class Executor = net::any_io_executor >
struct basic_semaphore
{
    /// @brief The implementation type
    using implementation_type = detail::semaphore_impl;

    /// @brief The type of the default executor.
    using executor_type = Executor;

    /// Rebinds the socket type to another executor.
    template < typename Executor1 >
    struct rebind_executor
    {
        /// The socket type when rebound to the specified executor.
        typedef basic_semaphore< Executor1 > other;
    };

    /// @brief Construct a semaphore
    /// @param exec is the default executor associated with the async_semaphore
    /// @param initial_count is the initial value of the internal counter.
    /// @pre initial_count >= 0
    /// @pre initial_count <= MAX_INT
    ///
    basic_semaphore(executor_type exec, int initial_count = 1);

    /// @brief Rebind a semaphore to a new executor - this cancels all outstanding operations.
    template<typename Executor_>
    basic_semaphore(basic_semaphore<Executor_> && sem,
                    std::enable_if_t<std::is_convertible<Executor_, executor_type>::value> * = nullptr)
        : exec_(sem.get_executor()), impl_(std::move(sem.impl_))
    {
    }

    /// Move assign a semaphore.
    basic_semaphore& operator=(basic_semaphore&&) noexcept = default;

    /// Move assign a semaphore with a different executor.
    template<typename Executor_>
    auto operator=(basic_semaphore<Executor_> && sem)
        -> std::enable_if_t<std::is_convertible<Executor_, executor_type>::value, basic_semaphore>  &
    {
        exec_ = std::move(sem.exec_);
        impl_ = std::move(sem.impl_);
        return *this;
    }

    basic_semaphore& operator=(const basic_semaphore&) = delete;

    /// The destructor. @param ctx The execution context used by the semaphore.
    template<typename ExecutionContext>
    explicit basic_semaphore(
            ExecutionContext & ctx,
            int initial_count = 1,
            typename std::enable_if<
                    std::is_convertible<
                            ExecutionContext&,
                            net::execution_context&>::value
            >::type * = nullptr)
        : exec_(ctx.get_executor())
        , impl_(ctx, initial_count)
    {
    }

    /// @brief return the default executor.
    executor_type
    get_executor() const noexcept;

    /// @brief Initiate an asynchronous acquire of the semaphore
    /// @details Multiple asynchronous acquire operations may be in progress at
    /// the same time. However, the caller must ensure that this function is not
    /// invoked from two threads simultaneously. When the semaphore's internal
    /// count is above zero, async acquire operations will complete in strict
    /// FIFO order. If the semaphore object is destoyed while an async_acquire
    /// is outstanding, the operation's completion handler will be invoked with
    /// the error_code set to error::operation_aborted. If the async_acquire
    /// operation is cancelled before completion, the completion handler will be
    /// invoked with the error_code set to error::operation_aborted. Successful
    /// acquisition of the semaphore is signalled to the caller when the
    /// completion handler is invoked with no error.
    /// @tparam CompletionHandler represents a completion token or handler which
    /// is invokable with the signature `void(error_code)`
    /// @param token is a completion token or handler matching the signature
    /// void(error_code)
    /// @note The completion handler will be invoked as if by `post` to the
    /// handler's associated executor. If no executor is associated with the
    /// completion handler, the handler will be invoked as if by `post` to the
    /// async_semaphore's associated default executor.
    template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionHandler
                BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
    BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionHandler, void(error_code))
    async_acquire( CompletionHandler &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(executor_type));

    /** Acquire synchronously. This may fail depending on the implementation.
    *
    * If the implementation is `st` this will generate an error if the semaphore
    * cannot be acquired immediately.
    *
    * If the implementation is `mt` this function will block until another thread releases
    * the semaphore. Note that this may lead to deadlocks.
    *
    * You should never use the synchronous functions from within an asio event-queue.
    *
    */
    void acquire(error_code & ec)
    {
        impl_.acquire(ec);
    }

    /// Throwing @overload lock(error_code &);
    void acquire()
    {
        error_code ec;
        acquire(ec);
        if (ec)
            throw system_error(ec, "acquire");
    }

    /// @brief Attempt to immediately acquire the semaphore.
    /// @details This function attempts to acquire the semaphore without
    /// blocking or initiating an asynchronous operation.
    /// @returns true if the semaphore was acquired, false otherwise
    BOOST_ASEM_DECL bool
    try_acquire()
    {
        return impl_.try_acquire();
    }

    /// @brief Release the sempahore.
    /// @details This function immediately releases the semaphore. If there are
    /// pending async_acquire operations, then the least recent operation will
    /// commence completion.
    BOOST_ASEM_DECL void
    release()
    {
        impl_.release();
    }

    /// The current value of the semaphore
    BOOST_ASEM_NODISCARD BOOST_ASEM_DECL int
    value() const noexcept
    {
        return impl_.value();
    }
  private:
    template<typename>
    friend struct basic_semaphore;

    executor_type exec_;
    implementation_type impl_;
    struct async_aquire_op;
};


BOOST_ASEM_END_NAMESPACE

#include <boost/asem/impl/basic_semaphore.hpp>

#endif