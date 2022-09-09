// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASEM_BASIC_SEMAPHORE_HPP
#define BOOST_ASEM_BASIC_SEMAPHORE_HPP


#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/bilist_node.hpp>

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
struct st;
struct mt;

namespace detail
{

template<typename Impl>
struct semaphore_impl;

}

template < class Implementation,
           class Executor = BOOST_ASEM_ASIO_NAMESPACE::any_io_executor >
struct basic_semaphore
{
    /// @brief The implementation type
    using implementation_type = detail::semaphore_impl<Implementation>;

    /// @brief The type of the default executor.
    using executor_type = Executor;

    /// Rebinds the socket type to another executor.
    template < typename Executor1 >
    struct rebind_executor
    {
        /// The socket type when rebound to the specified executor.
        typedef basic_semaphore< Implementation, Executor1 > other;
    };

    /// @brief Construct an async_sempaphore
    /// @param exec is the default executor associated with the async_semaphore
    /// @param initial_count is the initial value of the internal counter.
    /// @pre initial_count >= 0
    /// @pre initial_count <= MAX_INT
    ///
    basic_semaphore(executor_type exec, int initial_count = 1);

    /// @brief return the default executor.
    executor_type const &
    get_executor() const;

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
    implementation_type impl_;
    executor_type exec_;
    struct async_aquire_op;
};


BOOST_ASEM_END_NAMESPACE

#include <boost/asem/impl/basic_semaphore.hpp>

#endif