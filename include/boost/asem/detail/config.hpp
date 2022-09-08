// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_DETAIL_CONFIG_HPP
#define BOOST_ASEM_DETAIL_CONFIG_HPP

#if defined(BOOST_ASEM_STANDALONE)

#define BOOST_ASEM_ASIO_NAMESPACE ::asio
#define BOOST_ASEM_NODISCARD ASIO_NODISCARD
#define BOOST_ASEM_COMPLETION_TOKEN_FOR(Sig) ASIO_COMPLETION_TOKEN_FOR(Sig)
#define BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor) ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)
#define BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(Token, Signature) ASIO_INITFN_AUTO_RESULT_TYPE(Token, Signature)
#define BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(Executor) ASIO_DEFAULT_COMPLETION_TOKEN(Executor)


#include <asio/error.hpp>
#include <system_error>

#define BOOST_ASEM_BEGIN_NAMESPACE namespace asem {
#define BOOST_ASEM_END_NAMESPACE  }
#define BOOST_ASEM_NAMESPACE asem
#define BOOST_ASEM_ASSERT(Condition) ASIO_ASSERT(Condition)

#else

#define BOOST_ASEM_ASIO_NAMESPACE ::boost::asio
#define BOOST_ASEM_NODISCARD BOOST_ATTRIBUTE_NODISCARD

#define BOOST_ASEM_COMPLETION_TOKEN_FOR(Sig) BOOST_ASIO_COMPLETION_TOKEN_FOR(Sig)
#define BOOST_ASEM_DEFAULT_COMPLETION_TOKEN_TYPE(Executor) BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)
#define BOOST_ASEM_INITFN_AUTO_RESULT_TYPE(Token, Signature) BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(Token, Signature)
#define BOOST_ASEM_DEFAULT_COMPLETION_TOKEN(Executor) BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor)


#include <boost/config.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_category.hpp>
#include <boost/system/system_error.hpp>

#if defined(BOOST_WINDOWS_API)
#define BOOST_ASEM_WINDOWS 1

// Windows: suppress definition of "min" and "max" macros.
#if !defined(NOMINMAX)
# define NOMINMAX 1
#endif

#endif

#if defined(BOOST_POSIX_API)
#define BOOST_ASEM_POSIX 1
#endif

#if !defined(BOOST_ASEM_WINDOWS) && !defined(BOOST_POSIX_API)
#error Unsupported operating system
#endif

#if defined(BOOST_PROCESS_USE_STD_FS)
#include <filesystem>
#include <optional>
#else
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/optional.hpp>
#endif

#define BOOST_ASEM_BEGIN_NAMESPACE namespace boost { namespace asem {
#define BOOST_ASEM_END_NAMESPACE  } }
#define BOOST_ASEM_NAMESPACE boost::asem
#define BOOST_ASEM_ASSERT(Condition) BOOST_ASSERT(Condition)

#endif

BOOST_ASEM_BEGIN_NAMESPACE

#if defined(BOOST_ASEM_STANDALONE)

using std::error_code ;
using std::error_category ;
using std::system_category ;
using std::system_error ;

#else

using boost::system::error_code ;
using boost::system::error_category ;
using boost::system::system_category ;
using boost::system::system_error ;


#endif

BOOST_ASEM_END_NAMESPACE

#ifndef BOOST_ASEM_HEADER_ONLY
# ifndef BOOST_ASEM_SEPARATE_COMPILATION
#   define BOOST_ASEM_HEADER_ONLY 1
# endif
#endif

#if BOOST_ASEM_DOXYGEN
# define BOOST_ASEM_DECL
#elif defined(BOOST_ASEM_HEADER_ONLY)
# define BOOST_ASEM_DECL inline
#else
# define BOOST_ASEM_DECL
#endif

#endif //BOOST_ASEM_DETAIL_CONFIG_HPP
