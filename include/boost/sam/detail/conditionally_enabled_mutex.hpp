// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_CONDITIONALLY_ENABLED_MUTEX_HPP
#define BOOST_SAM_DETAIL_CONDITIONALLY_ENABLED_MUTEX_HPP

#include <boost/sam/detail/config.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

struct conditionally_enabled_mutex
{
  using scoped_lock = std::unique_lock<conditionally_enabled_mutex>;

  conditionally_enabled_mutex(bool enabled) : enabled_(enabled) {}
  void lock()
  {
    if (enabled_)
      mtx_.lock();
    locked_ = true;
  }

  void unlock()
  {
    locked_ = false;
    if (enabled_)
      mtx_.unlock();
  }

  bool enabled() const {return enabled_;}

private:
  bool enabled_ = false, locked_ = false;
  internal_mutex mtx_;
};

}
BOOST_SAM_END_NAMESPACE

#endif //BOOST_SAM_CONDITIONALLY_ENABLED_MUTEX_HPP
