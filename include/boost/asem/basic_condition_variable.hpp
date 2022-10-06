// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_BASIC_CONDITION_VARIABLE_HPP
#define BOOST_ASEM_BASIC_CONDITION_VARIABLE_HPP

#include <boost/asem/detail/config.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/any_io_executor.hpp>
#include <asio/async_result.hpp>
#else
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

struct st;
struct mt;

namespace detail
{

template<typename Impl>
struct condition_variable_impl;

}

/** An asio based condition variable modeled on `std::condition_variable`.
 *
 * @tparam Implementation The implementation, st or mt.
 * @tparam Executor The executor to use as default completion.
 */
template<typename Implementation, typename Executor = BOOST_ASEM_ASIO_NAMESPACE::any_io_executor>
struct basic_condition_variable
{
    /// The executor type.
    using executor_type = Executor;

    /// The destructor. @param exec The executor to be used by the condition variable
    explicit basic_condition_variable(executor_type exec)
            : exec_(std::move(exec))
    {
    }

    /// The destructor. @param ctx The execution context used by the condition variable.
    template<typename ExecutionContext>
    explicit basic_condition_variable(
                        ExecutionContext & ctx,
                        typename std::enable_if<
                            std::is_convertible<
                                     ExecutionContext&,
                                     BOOST_ASEM_ASIO_NAMESPACE::execution_context&>::value
                         >::type * = nullptr)
            : exec_(ctx.get_executor())
    {
    }

    /// @brief Rebind a condition_variable to a new executor - this cancels all outstanding operations.
    template<typename Executor_>
    basic_condition_variable(basic_condition_variable<Implementation, Executor_> && sem,
                             std::enable_if_t<std::is_convertible_v<Executor_, executor_type>> * = nullptr)
            : exec_(sem.get_executor())
    {
    }

    /** Wait for the condition_variable to become notified.
     *
     * @tparam CompletionToken The completion token type.
     * @param token The token for completion.
     * @return Deduced from the token.
     */
    template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionToken
        BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
            BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
    async_wait(CompletionToken &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return BOOST_ASEM_ASIO_NAMESPACE::async_initiate<CompletionToken, void(error_code)>(
                async_wait_op{this}, token);
    }

    /** Wait for the condition_variable to become notified & the predicate to return true.
     *
     * The async_wait completes immediately if the condition is true when calling.
     *
     * @tparam CompletionToken The completion token type.
     * @param token The token for completion.
     * @return Deduced from the token.
     */
    template < typename Predicate,
               BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionToken
                BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
                    BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
    async_wait(Predicate && predicate,
               CompletionToken &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(executor_type),
               typename std::enable_if<std::is_same<decltype(std::declval<Predicate>()()), bool>::value>::type * = nullptr)
    {
        return BOOST_ASEM_ASIO_NAMESPACE::async_initiate<CompletionToken, void(error_code)>(
                async_predicate_wait_op<
                        typename std::decay<Predicate>::type
                        >{this, std::forward<Predicate>(predicate)}, token);
    }

    /// Notify/wake up one waiting operations.
    void
    notify_one()
    {
        impl_.notify_one();
    }

    /// Notify/wake up all waiting operations.
    void
    notify_all()
    {
        impl_.notify_all();
    }

    /// Rebinds the mutex type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The mutex type when rebound to the specified executor.
        typedef basic_condition_variable<Implementation, Executor1> other;
    };

    /// @brief return the default executor.
    executor_type
    get_executor() const noexcept {return exec_;}

  private:
    detail::condition_variable_impl<Implementation> impl_;
    Executor exec_;


    template<typename Predicate>
    struct async_predicate_wait_op;
    struct async_wait_op;

};

BOOST_ASEM_END_NAMESPACE

#include <boost/asem/impl/basic_condition_variable.hpp>


#endif //BOOST_ASEM_BASIC_CONDITION_VARIABLE_HPP
