// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_BASIC_BARRIER_HPP
#define BOOST_ASEM_BASIC_BARRIER_HPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/service.hpp>
#include <boost/asem/detail/barrier_impl.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/any_io_executor.hpp>
#include <asio/compose.hpp>

#else
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/compose.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

/** An asio based barrier modeled on `std::barrier`.
 *
 * @tparam Executor The executor to use as default completion.
 */
template<typename Executor = net::any_io_executor>
struct basic_barrier
{
    /// The executor type.
    using executor_type = Executor;

    /// The constructor.
    /// @param exec The executor to be used by the barrier.
    /// @param init_count The number of thread for the barrier.
    explicit basic_barrier(executor_type exec, std::ptrdiff_t init_count)
            : exec_(std::move(exec))
            , impl_{net::query(exec_, net::execution::context), init_count}
    {
    }

    /// A constructor.
    // @param ctx The execution context used by the barrier.
    template<typename ExecutionContext>
    explicit basic_barrier(ExecutionContext & ctx,
                         typename std::enable_if<
                                 std::is_convertible<
                                         ExecutionContext&,
                                         net::execution_context&>::value,
                                     std::ptrdiff_t
                                 >::type init_count)
            : exec_(ctx.get_executor()), impl_{ctx, init_count}
    {
    }

    /// Rebind a barrier to a new executor - this cancels all outstanding operations.
    template<typename Executor_>
    basic_barrier(basic_barrier<Executor_> && sem,
                  std::enable_if_t<std::is_convertible<Executor_, executor_type>::value> * = nullptr)
            : exec_(sem.get_executor()), impl_(std::move(sem.impl_))
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
        return net::async_initiate<CompletionToken, void(std::error_code)>(
                async_arrive_op{this}, token);
    }

    /// Move assign a barrier.
    basic_barrier& operator=(basic_barrier&&) noexcept = default;

    /// Move assign a barrier with a different executor.
    template<typename Executor_>
    auto operator=(basic_barrier<Executor_> && sem)
        ->std::enable_if_t<std::is_convertible<Executor_, executor_type>::value, basic_barrier>  &
    {
        exec_ = std::move(sem.exec_);
        impl_ = std::move(sem.impl_);
        return *this;
    }

    /// Delete copy assignment
    basic_barrier& operator=(const basic_barrier&) = delete;

    /// Try to arrive - that is arrive immediately if we're the last thread.
    bool try_arrive()
    {
        return impl_.try_arrive();
    }

    /** Arrive synchronously. This may fail depending on the implementation.
     *
     * If the implementation is `st` this will generate an error if the barrier
     * is not ready.
     *
     * If the implementation is `mt` this function will block until other threads arrive.
     * Note that this may lead to deadlocks.
     *
     * You should never use the synchronous functions from within an asio event-queue.
     *
     */
    void arrive(error_code & ec)
    {
        impl_.arrive(ec);
    }

    /// Throwing @overload arrive(error_code &);
    void arrive()
    {
        error_code ec;
        arrive(ec);
        if (ec)
            throw system_error(ec, "arrive");
    }

    /// Rebinds the barrier type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The barrier type when rebound to the specified executor.
        typedef basic_barrier<Executor1> other;
    };

    /// @brief return the default executor.
    executor_type
    get_executor() const noexcept {return exec_;}

  private:
    template<typename>
    friend struct basic_barrier;

    Executor exec_;
    detail::barrier_impl impl_;
    struct async_arrive_op;
};


BOOST_ASEM_END_NAMESPACE

#include <boost/asem/impl/basic_barrier.hpp>

#endif //BOOST_ASEM_BASIC_BARRIER_HPP
