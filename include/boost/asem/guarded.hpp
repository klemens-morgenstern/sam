// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_GUARDED_HPP
#define BOOST_ASEM_GUARDED_HPP

#include <boost/asem/detail/config.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/async_result.hpp>
#include <asio/associator.hpp>
#include <asio/prepend.hpp>
#include <asio/cancellation_signal.hpp>
#include <asio/deferred.hpp>
#else
#include <boost/asio/associator.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/prepend.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

template<typename Implementation, typename Execution>
struct basic_semaphore;
template<typename Implementation, typename Execution>
struct basic_mutex;

template<typename Mutex>
struct lock_guard;

namespace detail
{

template<typename Implementation, typename Executor, typename Op, typename Signature>
struct guard_by_semaphore_op;

template<typename Implementation, typename Executor, typename Op, typename Err, typename ... Args>
struct guard_by_semaphore_op<Implementation, Executor, Op, void (Err, Args...)>
{
    basic_semaphore<Implementation, Executor> & sm;
    Op op;

    struct semaphore_tag {};
    struct op_tag {};

    static auto make_error_impl(error_code ec, error_code *)
    {
        return ec;
    }

    static auto make_error_impl(error_code ec, std::exception_ptr *)
    {
        return std::make_exception_ptr(system_error(ec));
    }

    static auto make_error(error_code ec)
    {
        return make_error_impl(ec, static_cast<Err*>(nullptr));
    }

    template<typename Self>
    void operator()(Self && self) // init
    {
        if (self.get_cancellation_state().cancelled() != BOOST_ASEM_ASIO_NAMESPACE::cancellation_type::none)
            return std::move(self).complete(make_error(BOOST_ASEM_ASIO_NAMESPACE::error::operation_aborted), Args{}...);

        auto h2 =  BOOST_ASEM_ASIO_NAMESPACE::prepend(std::move(self), semaphore_tag{});
        sm.async_acquire(std::move(h2));
    }

    template<typename Self>
    void operator()(Self && self, semaphore_tag, error_code ec) // semaphore obtained
    {
        if (ec)
            self.complete(make_error(ec), Args{}...);
        else
            std::move(op)(BOOST_ASEM_ASIO_NAMESPACE::prepend(std::move(self), op_tag{}));
    }

    template<typename Self, typename ... Args_>
    void operator()(Self && self, op_tag, Args_ &&  ... args ) // semaphore obtained
    {
        sm.release();
        std::move(self).complete(std::forward<Args_>(args)...);
    }
};

}

/** Function to run OPs only when the semaphore can be acquired.
 *  That way an artificial number of processes can run in parallel.
 *
 *  @tparam Implementation The implementation of the semaphore, i.e. `st` or `mt`.
 *  @tparam Executor The executor of the semaphore.
 *  @tparam token The completion token
 *
 *  @param sm The semaphore to guard the protection
 *  @param op The operation to guard.
 *  @param completion_token The completion token to use for the async completion.
 */
template<typename Implementation,
         typename Executor, typename Op,
        BOOST_ASEM_COMPLETION_TOKEN_FOR(
                typename BOOST_ASEM_ASIO_NAMESPACE::completion_signature_of<Op>::type) CompletionToken
        BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
auto guarded(basic_semaphore<Implementation, Executor> & sm, Op && op,
             CompletionToken && completion_token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(Executor))
{
    using sig_t = typename decltype(std::declval<Op>()(BOOST_ASEM_ASIO_NAMESPACE::detail::completion_signature_probe{}))::type;
    using cop = detail::guard_by_semaphore_op<Implementation, Executor, std::decay_t<Op>, sig_t>;
    return BOOST_ASEM_ASIO_NAMESPACE::async_compose<CompletionToken, sig_t>(
            cop{sm, std::forward<Op>(op)}, completion_token, sm);
}

namespace detail
{
template<typename Implementation, typename Executor, typename Op, typename Signature>
struct guard_by_mutex_op;

template<typename Implementation, typename Executor, typename Op, typename Err, typename ... Args>
struct guard_by_mutex_op<Implementation, Executor, Op, void(Err, Args...)>
{
    basic_mutex<Implementation, Executor> &sm;
    Op op;

    struct semaphore_tag
    {
    };
    struct op_tag
    {
    };

    static auto make_error_impl(error_code ec, error_code *)
    {
        return ec;
    }

    static auto make_error_impl(error_code ec, std::exception_ptr *)
    {
        return std::make_exception_ptr(std::system_error(ec));
    }

    static auto make_error(error_code ec)
    {
        return make_error_impl(ec, static_cast<Err *>(nullptr));
    }

    template<typename Self>
    void operator()(Self &&self) // init
    {
        if (self.get_cancellation_state().cancelled() != BOOST_ASEM_ASIO_NAMESPACE::cancellation_type::none)
            return std::move(self).complete(make_error(BOOST_ASEM_ASIO_NAMESPACE::error::operation_aborted), Args{}...);

        sm.async_lock(BOOST_ASEM_ASIO_NAMESPACE::prepend(std::move(self), semaphore_tag{}));
    }

    template<typename Self>
    void
    operator()(Self &&self, semaphore_tag, error_code ec) // semaphore obtained
    {
        if (ec)
            self.complete(make_error(ec), Args{}...);
        else
            std::move(op)(BOOST_ASEM_ASIO_NAMESPACE::prepend(std::move(self), op_tag{}));
    }

    template<typename Self, typename ... Args_>
    void operator()(Self &&self, op_tag, Args_ &&... args) // semaphore obtained
    {
        sm.unlock();
        std::move(self).complete(std::forward<Args_>(args)...);
    }
};

}


/** Function to run OPs only when the mutex can be locked.
 * Unlocks the mutex on completion.
 *
 *  @tparam Implementation The implementation of the mutex, i.e. `st` or `mt`.
 *  @tparam Executor The executor of the semaphore.
 *  @tparam token The completion token
 *
 *  @param sm The mutex to guard the protection
 *  @param op The operation to guard.
 *  @param completion_token The completion token to use for the async completion.
 */
template<typename Implementation,
        typename Executor, typename Op,
        BOOST_ASEM_COMPLETION_TOKEN_FOR(
                typename BOOST_ASEM_ASIO_NAMESPACE::completion_signature_of<Op>::type) CompletionToken
        BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
auto guarded(basic_mutex<Implementation, Executor> & mtx,
             Op && op,
             CompletionToken && completion_token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(Executor))
{
    using sig_t = typename decltype(std::declval<Op>()(BOOST_ASEM_ASIO_NAMESPACE::detail::completion_signature_probe{}))::type;
    using cop = detail::guard_by_mutex_op<Implementation, Executor, std::decay_t<Op>, sig_t>;
    return BOOST_ASEM_ASIO_NAMESPACE::async_compose<CompletionToken, sig_t>(
            cop{mtx, std::forward<Op>(op)}, completion_token, mtx);
}

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_GUARDED_HPP
