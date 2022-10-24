// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_LOCK_GUARD_HPP
#define BOOST_ASEM_LOCK_GUARD_HPP

#include <boost/asem/detail/config.hpp>
#include <utility>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/async_result.hpp>
#include <asio/deferred.hpp>
#else
#include <boost/asio/async_result.hpp>
#include <boost/asio/deferred.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

template<typename Implementation, typename Executor>
struct basic_mutex;

/** A lock-guard used as an RAII object that automatically unlocks on destruction
 *
 * To use with with async_clock.
 *
 *@tparam Mutex The underlying `basic_mutex`.
 *
 */
template<typename Mutex>
struct lock_guard
{
    /// Construct an empty lock_guard.
    lock_guard() = default;
    lock_guard(const lock_guard &) = delete;
    lock_guard(lock_guard &&lhs) : mtx_(lhs.mtx_)
    {
        lhs.mtx_ = nullptr;
    }

    lock_guard &
    operator=(const lock_guard &) = delete;
    lock_guard &
    operator=(lock_guard &&lhs)
    {
        std::swap(lhs.mtx_, mtx_);
        return *this;
    }

    /// Unlock the underlying mutex.
    ~lock_guard()
    {
        if (mtx_ != nullptr)
            mtx_->unlock();
    }
    template<typename Implementation, typename Executor,
            BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code, lock_guard<basic_mutex<Implementation, Executor>>)) CompletionHandler>
    friend BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionHandler, void(error_code, lock_guard<basic_mutex<Implementation, Executor>>))
        async_lock(basic_mutex<Implementation, Executor> &mtx, CompletionHandler &&token);

    template<typename Implementation, typename Executor>
    friend lock_guard<basic_mutex<Implementation, Executor>> lock(basic_mutex<Implementation, Executor> & mtx);

    template<typename Implementation, typename Executor>
    friend lock_guard<basic_mutex<Implementation, Executor>> lock(basic_mutex<Implementation, Executor> & mtx, error_code & ec);

    lock_guard(Mutex & mtx, const std::adopt_lock_t &) : mtx_(&mtx) {}

  private:
    lock_guard(Mutex *mtx) : mtx_(mtx)
    {
    }

    Mutex * mtx_ = nullptr;
};

/** Acquire a lock_guard synchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The lock_guard.
 *
 * @throws May throw a system_error if locking is not possible without a deadlock.
 */
template<typename Implementation, typename Executor>
lock_guard<basic_mutex<Implementation, Executor>> lock(basic_mutex<Implementation, Executor> & mtx)
{
    mtx.lock();
    return lock_guard<basic_mutex<Implementation, Executor>>(&mtx);
}


/** Acquire a lock_guard synchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The lock_guard. It might be default constructed if locking   wasn't possible.
 */
template<typename Implementation, typename Executor>
lock_guard<basic_mutex<Implementation, Executor>> lock(basic_mutex<Implementation, Executor> & mtx,
                                                       error_code & ec)
{
    mtx.lock(ec);
    if (ec)
        return lock_guard<basic_mutex<Implementation, Executor>>();
    else
        return lock_guard<basic_mutex<Implementation, Executor>>(&mtx);
}


/** Acquire a lock_guard asynchronously.
 *
 * @param mtx The mutex to lock.
 * @param token The Completion Token.
 *
 * @returns The async_result deduced from the token.
 *
 * @tparam Implementation The mutex implementation
 * @tparam Executor The executor type of the mutex
 * @tparam CompletionToken The completion token.
 *
 * @example
 * @code{.cpp}
 *
 * awaitable<std::string> protected_read(st::mutex & mtx, tcp::socker & sock)
 * {
 *  std::string buf;
 *  auto l = co_await async_lock(mtx);
 *  co_await socket.async_read(dynamic_buffer(buf), use_awaitable);
 * }
 *
 * @endcode
 *
 */
template<typename Implementation, typename Executor,
         BOOST_ASEM_COMPLETION_TOKEN_FOR(void(error_code, lock_guard<basic_mutex<Implementation, Executor>>)) CompletionToken
             BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor) >
inline BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code, lock_guard<Mutex>))
    async_lock(basic_mutex<Implementation, Executor> &mtx,
                CompletionToken &&token BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(Executor))
{
    using BOOST_ASEM_ASIO_NAMESPACE::deferred;
    using lg_t = lock_guard<basic_mutex<Implementation, Executor>>;
    return mtx.async_lock(
            deferred([&](error_code ec)
            {
               if (ec)
                   return deferred.values(ec, lg_t{});
               else
                   return deferred.values(ec, lg_t{&mtx});
            }))(std::forward<CompletionToken>(token));
}

BOOST_ASEM_END_NAMESPACE

#endif //BOOST_ASEM_LOCK_GUARD_HPP
