// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ASEM_MT_IMPL_BASIC_SEMAPHORE_IPP
#define BOOST_ASEM_MT_IMPL_BASIC_SEMAPHORE_IPP

#include <boost/asem/detail/config.hpp>
#include <boost/asem/detail/basic_op.hpp>
#include <boost/asem/mt/basic_semaphore.hpp>

#include <condition_variable>

BOOST_ASEM_BEGIN_NAMESPACE
namespace detail
{

semaphore_impl<mt>::semaphore_impl(int initial_count)
    : waiters_(), count_(initial_count)
{
}

void
semaphore_impl<mt>::add_waiter(detail::wait_op *waiter) noexcept
{
    waiter->link_before(&waiters_);
}

int
semaphore_impl<mt>::count() const noexcept
{
    return count_.load();
}

void
semaphore_impl<mt>::release()
{
    std::lock_guard<std::mutex> lock_{mtx_};
    count_ ++;

    // release a pending operations
    if (waiters_.next_ == &waiters_)
        return;

    decrement();
    static_cast< detail::wait_op * >(waiters_.next_)->complete(std::error_code());
}



void
semaphore_impl<mt>::acquire(error_code & ec)
{
    if (try_acquire())
        return ;
    struct op_t final : detail::wait_op
    {
        error_code ec;
        semaphore_impl<mt> * this_;
        bool done = false;
        std::condition_variable var;
        op_t(error_code & ec, semaphore_impl<mt> * this_) : ec(ec), this_(this_) {}

        void complete(error_code ec) override
        {
            done = true;
            this->ec = ec;
            var.notify_all();
            this_->decrement();
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

    op_t op{ec, this};
    std::unique_lock<std::mutex> lock(mtx_);
    add_waiter(&op);
    op.wait(lock);
}

BOOST_ASEM_NODISCARD int
semaphore_impl<mt>::value() const noexcept
{
    std::lock_guard<std::mutex> lock_{mtx_};
    if (waiters_.next_ == &waiters_)
        return count();

    return count() - static_cast<int>(waiters_.size());
}

bool
semaphore_impl<mt>::try_acquire()
{
    if (count_.fetch_sub(1) >= 0)
        return true;
    else
        count_++;
    return false;
}

int
semaphore_impl<mt>::decrement()
{
    //BOOST_ASEM_ASSERT(count_ > 0);
    return --count_;
}

BOOST_ASEM_DECL std::unique_lock<std::mutex> semaphore_impl<mt>::internal_lock()
{
    return std::unique_lock<std::mutex>{mtx_};
}

}
BOOST_ASEM_END_NAMESPACE


#endif //BOOST_ASEM_MT_IMPL_BASIC_SEMAPHORE_IPP
