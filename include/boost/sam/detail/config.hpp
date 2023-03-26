// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_CONFIG_HPP
#define BOOST_SAM_DETAIL_CONFIG_HPP

#if defined(BOOST_SAM_STANDALONE)

#define BOOST_SAM_NODISCARD                                 ASIO_NODISCARD
#define BOOST_SAM_COMPLETION_TOKEN_FOR(Sig)                 ASIO_COMPLETION_TOKEN_FOR(Sig)
#define BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)   ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)
#define BOOST_SAM_INITFN_AUTO_RESULT_TYPE(Token, Signature) ASIO_INITFN_AUTO_RESULT_TYPE(Token, Signature)
#define BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor)        ASIO_DEFAULT_COMPLETION_TOKEN(Executor)

#include <asio/detail/conditionally_enabled_mutex.hpp>
#include <asio/detail/event.hpp>
#include <asio/error.hpp>
#include <system_error>

#define BOOST_SAM_BEGIN_NAMESPACE                                                                                      \
  namespace sam                                                                                                        \
  {
#define BOOST_SAM_END_NAMESPACE        }
#define BOOST_SAM_NAMESPACE            sam
#define BOOST_SAM_ASSERT(Condition)    ASIO_ASSERT(Condition)
#define BOOST_SAM_ASSIGN_EC(ec, error) ec = error

#else

#define BOOST_SAM_NODISCARD BOOST_ATTRIBUTE_NODISCARD

#define BOOST_SAM_COMPLETION_TOKEN_FOR(Sig)                 BOOST_ASIO_COMPLETION_TOKEN_FOR(Sig)
#define BOOST_SAM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)   BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)
#define BOOST_SAM_INITFN_AUTO_RESULT_TYPE(Token, Signature) BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(Token, Signature)
#define BOOST_SAM_DEFAULT_COMPLETION_TOKEN(Executor)        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor)

#include <boost/asio/detail/conditionally_enabled_mutex.hpp>
#include <boost/asio/detail/event.hpp>
#include <boost/asio/error.hpp>
#include <boost/config.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_category.hpp>
#include <boost/system/system_error.hpp>

#if defined(BOOST_WINDOWS_API)
#define BOOST_SAM_WINDOWS 1

// Windows: suppress definition of "min" and "max" macros.
#if !defined(NOMINMAX)
#define NOMINMAX 1
#endif

#endif

#if defined(BOOST_POSIX_API)
#define BOOST_SAM_POSIX 1
#endif

#if !defined(BOOST_SAM_WINDOWS) && !defined(BOOST_POSIX_API)
#error Unsupported operating system
#endif

#define BOOST_SAM_BEGIN_NAMESPACE                                                                                      \
  namespace boost                                                                                                      \
  {                                                                                                                    \
  namespace sam                                                                                                        \
  {
#define BOOST_SAM_END_NAMESPACE                                                                                        \
  }                                                                                                                    \
  }
#define BOOST_SAM_NAMESPACE         boost::sam
#define BOOST_SAM_ASSERT(Condition) BOOST_ASSERT(Condition)
#define BOOST_SAM_ASSIGN_EC(ec, ev)                                                                                    \
  do                                                                                                                   \
  {                                                                                                                    \
    static constexpr auto loc##__LINE__((BOOST_CURRENT_LOCATION));                                                     \
    ec = ::boost::system::error_code((ev), &loc##__LINE__);                                                            \
  } while (false)
#endif

BOOST_SAM_BEGIN_NAMESPACE

#if defined(BOOST_SAM_STANDALONE)

using std::error_category;
using std::error_code;
using std::system_category;
using std::system_error;
template <typename T>
inline void ignore_unused(const T &)
{
}

namespace net = ::asio;

#else

using boost::ignore_unused;
using boost::system::error_category;
using boost::system::error_code;
using boost::system::system_category;
using boost::system::system_error;

namespace net = ::boost::asio;

#endif

namespace detail
{
// maybe ported over.
using net::detail::conditionally_enabled_mutex;
using net::detail::event;
} // namespace detail

BOOST_SAM_END_NAMESPACE

#ifndef BOOST_SAM_HEADER_ONLY
#ifndef BOOST_SAM_SEPARATE_COMPILATION
#define BOOST_SAM_SEPARATE_COMPILATION 1
#endif
#endif

#if BOOST_SAM_DOXYGEN
#define BOOST_SAM_DECL
#elif defined(BOOST_SAM_HEADER_ONLY)
#define BOOST_SAM_DECL inline
#else
#if !defined(BOOST_SAM_STANDALONE)
#define BOOST_SAM_DECL BOOST_SYMBOL_EXPORT
#else
#define BOOST_SAM_DECL
#endif

#endif

#endif // BOOST_SAM_DETAIL_CONFIG_HPP
