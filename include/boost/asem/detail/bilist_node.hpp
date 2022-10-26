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
    bilist_node() noexcept : next_(this), prev_(this) {}
    bilist_node(bilist_node const &) = delete;

    bilist_node &
    operator=(bilist_node const &) = delete;

    ~bilist_node() noexcept = default;

    void unlink() noexcept
    {
        auto p   = prev_;
        auto n   = next_;
        n->prev_ = p;
        p->next_ = n;
    }

    void
    link_before(bilist_node *next) noexcept
    {
        next_        = next;
        prev_        = next->prev_;
        prev_->next_ = this;
        next->prev_  = this;
    }

    std::size_t size() const
    {
        std::size_t sz = 0;
        for (auto p = next_; p != this; p = p->next_)
            sz++;
        return sz;
    }

    bilist_node *next_;
    bilist_node *prev_;

    // for the root node
    bilist_node(bilist_node && lhs) noexcept
        : next_(lhs.next_)
        , prev_(lhs.prev_)
    {
        lhs.next_ = &lhs;
        lhs.prev_ = &lhs;

        if (next_ == &lhs)
            next_ = this;
        else
            next_->prev_ = this;

        if (prev_ == &lhs)
            prev_ = this;
        else
            prev_->next_ = this;
    }

    bilist_node& operator=(bilist_node && lhs)
    {
        if (lhs.next_ == &lhs)
            next_ = this;
        else
            next_->prev_ = this;

        if (lhs.prev_ == &lhs)
            prev_ = this;
        else
            prev_->next_ = this;
        return *this;
    }
};

}   // namespace detail
BOOST_ASEM_END_NAMESPACE

#endif
