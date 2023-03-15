// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_GUARDED_HPP
#define BOOST_SAM_DETAIL_GUARDED_HPP

#include <boost/sam/detail/config.hpp>

#if defined(BOOST_SAM_STANDALONE)
#include <asio/compose.hpp>
#include <asio/prepend.hpp>
#include <asio/cancellation_type.hpp>
#else
#include <boost/asio/compose.hpp>
#include <boost/asio/prepend.hpp>
#include <boost/asio/cancellation_type.hpp>
#endif

BOOST_SAM_BEGIN_NAMESPACE

template<typename>
struct basic_semaphore;
template<typename>
struct basic_mutex;

struct lock_guard;

namespace detail
{

template<typename Executor, typename Op, typename Signature>
struct guard_by_semaphore_op;

template<typename Executor, typename Op, typename Err, typename ... Args>
struct guard_by_semaphore_op<Executor, Op, void (Err, Args...)>
{
  basic_semaphore<Executor> & sm;
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
    if (self.get_cancellation_state().cancelled() != net::cancellation_type::none)
      return std::move(self).complete(make_error(net::error::operation_aborted), Args{}...);

    auto h2 =  net::prepend(std::move(self), semaphore_tag{});
    sm.async_acquire(std::move(h2));
  }

  template<typename Self>
  void operator()(Self && self, semaphore_tag, error_code ec) // semaphore obtained
  {
    if (ec)
      self.complete(make_error(ec), Args{}...);
    else
      std::move(op)(net::prepend(std::move(self), op_tag{}));
  }

  template<typename Self, typename ... Args_>
  void operator()(Self && self, op_tag, Args_ &&  ... args ) // semaphore obtained
  {
    sm.release();
    std::move(self).complete(std::forward<Args_>(args)...);
  }
};


template<typename Executor, typename Op, typename Signature>
struct guard_by_mutex_op;

template<typename Executor, typename Op, typename Err, typename ... Args>
struct guard_by_mutex_op<Executor, Op, void(Err, Args...)>
{
  basic_mutex<Executor> &sm;
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
    if (self.get_cancellation_state().cancelled() != net::cancellation_type::none)
      return std::move(self).complete(make_error(net::error::operation_aborted), Args{}...);

    sm.async_lock(net::prepend(std::move(self), semaphore_tag{}));
  }

  template<typename Self>
  void
  operator()(Self &&self, semaphore_tag, error_code ec) // semaphore obtained
  {
    if (ec)
      self.complete(make_error(ec), Args{}...);
    else
      std::move(op)(net::prepend(std::move(self), op_tag{}));
  }

  template<typename Self, typename ... Args_>
  void operator()(Self &&self, op_tag, Args_ &&... args) // semaphore obtained
  {
    sm.unlock();
    std::move(self).complete(std::forward<Args_>(args)...);
  }
};

}

BOOST_SAM_END_NAMESPACE

#endif //BOOST_SAM_DETAIL_GUARDED_HPP
