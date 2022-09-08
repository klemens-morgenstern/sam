//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_DETAIL_SEMAPHORE_WAIT_OP
#define BOOST_ASEM_DETAIL_SEMAPHORE_WAIT_OP

#include <boost/asem/detail/bilist_node.hpp>
#include <boost/asem/detail/config.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

struct semaphore_base;

namespace detail
{
struct semaphore_wait_op : detail::bilist_node
{
    semaphore_wait_op(semaphore_base *host) : host_(host) {}

    virtual void complete(error_code) = 0;

    semaphore_base *host_;
};

}   // namespace detail
BOOST_ASEM_END_NAMESPACE

#endif