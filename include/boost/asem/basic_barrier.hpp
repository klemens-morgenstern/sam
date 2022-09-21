// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_basic_barrier_HPP
#define BOOST_ASEM_basic_barrier_HPP

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
struct barrier_impl;

}

/** An asio based BARRIER modeled on `std::BARRIER`.
 *
 * @tparam Implementation The implementation, st or mt.
 * @tparam Executor The executor to use as default completion.
 */
template<typename Implementation, typename Executor = BOOST_ASEM_ASIO_NAMESPACE::any_io_executor>
struct basic_barrier
{
    /// The executor type.
    using executor_type = Executor;

    /// The destructor. @param exec The executor to be used by the BARRIER.
    explicit basic_barrier(executor_type exec, std::ptrdiff_t init_count)
            : exec_(std::move(exec)), impl_{init_count}
    {
    }

    /// The destructor. @param ctx The execution context used by the BARRIER.
    template<typename ExecutionContext>
    explicit basic_barrier(ExecutionContext & ctx,
                         typename std::enable_if<
                                 std::is_convertible<
                                         ExecutionContext&,
                                         BOOST_ASEM_ASIO_NAMESPACE::execution_context&>::value,
                                     std::ptrdiff_t
                                 >::type init_count)
            : exec_(ctx.get_executor()), impl_{init_count}
    {
    }


    /** Arrive at a barrier and wait for all other strands to arrive.
     *
     * @tparam CompletionToken The completion token type.
     * @param token The token for completion.
     * @return Deduced from the token.
     */
    template < BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code)) CompletionToken
        BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
        BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
    async_arrive(CompletionToken &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return BOOST_ASEM_ASIO_NAMESPACE::async_initiate<CompletionToken, void(std::error_code)>(
                async_arrive_op{this}, token);
    }

    bool try_arrive()
    {
        return impl_.try_arrive();
    }

    /// Rebinds the BARRIER type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The BARRIER type when rebound to the specified executor.
        typedef basic_barrier<Implementation, Executor1> other;
    };

    /// @brief return the default executor.
    executor_type
    get_executor() const noexcept {return exec_;}

  private:
    detail::barrier_impl<Implementation> impl_;
    Executor exec_;
    struct async_arrive_op;
};


BOOST_ASEM_END_NAMESPACE

#include <boost/asem/impl/basic_barrier.hpp>

#endif //BOOST_ASEM_basic_barrier_HPP
