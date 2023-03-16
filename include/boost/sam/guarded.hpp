// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_GUARDED_HPP
#define BOOST_SAM_GUARDED_HPP

#include <boost/sam/detail/guarded.hpp>

BOOST_SAM_BEGIN_NAMESPACE

/** Function to run OPs only when the semaphore can be acquired.
 *  That way an artificial number of processes can run in parallel.
 *
 *  @tparam Executor The executor of the semaphore.
 *  @tparam token The completion token
 *
 *  @param sm The semaphore to guard the protection
 *  @param op The operation to guard.
 *  @param completion_token The completion token to use for the async completion.
 */
template <typename Executor, typename Op,
          BOOST_SAM_COMPLETION_TOKEN_FOR(typename net::completion_signature_of<Op>::type)
              CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
auto guarded(basic_semaphore<Executor> &sm, Op &&op,
             CompletionToken &&completion_token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  using sig_t = typename decltype(std::declval<Op>()(net::detail::completion_signature_probe{}))::type;
  using cop   = detail::guard_by_semaphore_op<Executor, std::decay_t<Op>, sig_t>;
  return net::async_compose<CompletionToken, sig_t>(cop{sm, std::forward<Op>(op)}, completion_token, sm);
}

/** Function to run OPs only when the mutex can be locked.
 * Unlocks the mutex on completion.
 *
 *  @tparam Executor The executor of the semaphore.
 *  @tparam token The completion token
 *
 *  @param sm The mutex to guard the protection
 *  @param op The operation to guard.
 *  @param completion_token The completion token to use for the async completion.
 */
template <typename Executor, typename Op,
          BOOST_SAM_COMPLETION_TOKEN_FOR(typename net::completion_signature_of<Op>::type)
              CompletionToken BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
auto guarded(basic_mutex<Executor> &mtx, Op &&op,
             CompletionToken &&completion_token BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor))
{
  using sig_t = typename decltype(std::declval<Op>()(net::detail::completion_signature_probe{}))::type;
  using cop   = detail::guard_by_mutex_op<Executor, std::decay_t<Op>, sig_t>;
  return net::async_compose<CompletionToken, sig_t>(cop{mtx, std::forward<Op>(op)}, completion_token, mtx);
}

BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_GUARDED_HPP
