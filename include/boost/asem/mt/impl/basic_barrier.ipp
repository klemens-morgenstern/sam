// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_IMPL_BASIC_BARRIER_IPP
#define BOOST_ASEM_MT_IMPL_BASIC_BARRIER_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/mt/basic_barrier.hpp>
#include <condition_variable>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

bool barrier_impl<mt>::try_arrive()
{
    if (counter_.fetch_sub(1) <= 1)
    {
        auto l = internal_lock();
        waiters_.complete_all({});
        counter_ = init_;
        return true;
    }
    else
    {
        counter_ ++;
        return false;
    }

}

void barrier_impl<mt>::arrive(error_code &ec)
{
    if (try_arrive())
        return;

    struct op_t final : detail::wait_op
    {
        error_code ec;
        bool done = false;
        std::condition_variable var;
        op_t(error_code & ec) : ec(ec) {}

        void complete(error_code ec) override
        {
            done = true;
            this->ec = ec;
            var.notify_all();
            this->unlink();
        }

        void shutdown() override
        {
          done = true;
          this->ec = net::error::shut_down;
          var.notify_all();
          this->unlink();
        }

        ~op_t()
        {
        }

        void wait(std::unique_lock<std::mutex> & lock)
        {
            var.wait(lock, [this]{ return done;});
        }
    };

    op_t op{ec};
    std::unique_lock<std::mutex> lock(mtx_);
    decrement();
    add_waiter(&op);
    op.wait(lock);
}

void
barrier_impl<mt>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}


}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_BARRIER_IPP
