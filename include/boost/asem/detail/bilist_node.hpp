// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASEM_DETAIL_BILIST_NODE_HPP
#define BOOST_ASEM_DETAIL_BILIST_NODE_HPP


#include <boost/asem/detail/config.hpp>
#include <cstddef>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{
struct bilist_node
{
    inline bilist_node();

    bilist_node(bilist_node const &) = delete;

    bilist_node &
    operator=(bilist_node const &) = delete;

    inline ~bilist_node();

    inline void
    unlink();

    inline void
    link_before(bilist_node *next);

    inline std::size_t size() const;

    bilist_node *next_;
    bilist_node *prev_;
};

bilist_node::bilist_node()
: next_(this)
, prev_(this)
{
}

bilist_node::~bilist_node()
{
}

inline void
bilist_node::unlink()
{
    auto p   = prev_;
    auto n   = next_;
    n->prev_ = p;
    p->next_ = n;
}

void
bilist_node::link_before(bilist_node *next)
{
    next_        = next;
    prev_        = next->prev_;
    prev_->next_ = this;
    next->prev_  = this;
}

std::size_t
bilist_node::size() const
{
    std::size_t sz = 0;
    for (auto p = next_; p != this; p = p->next_)
        sz++;
    return sz;
}

}   // namespace detail
BOOST_ASEM_END_NAMESPACE

#endif
