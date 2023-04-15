// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_DETAIL_IMPL_CONDITION_VARIABLE_IMPL_IPP
#define BOOST_SAM_DETAIL_IMPL_CONDITION_VARIABLE_IMPL_IPP

#include <boost/sam/detail/config.hpp>
#include <boost/sam/detail/basic_op.hpp>
#include <boost/sam/detail/condition_variable_impl.hpp>

BOOST_SAM_BEGIN_NAMESPACE
namespace detail
{

condition_variable_impl::condition_variable_impl(net::execution_context &ctx,
                                                 int concurrency_hint) : detail::service_member(ctx, concurrency_hint)
{
}

void condition_variable_impl::add_waiter(detail::basic_op *waiter) noexcept
{
  waiter->link_before(&waiters_);
}

void condition_variable_impl::notify_one()
{
  lock_type lock{this->mtx_};
  // release a pending operations
  if (waiters_.next_ == &waiters_)
    return;
  auto op = static_cast<detail::basic_op *>(waiters_.next_);
  op->complete(error_code());
}

void condition_variable_impl::notify_all()
{
  lock_type lock{this->mtx_};
  // release a pending operations
  for (auto c = waiters_.next_; c != &waiters_;)
  {
    auto op = static_cast<detail::basic_op *>(c);
    c       = c->next_;
    op->complete(error_code());
  }
}


struct condition_variable_impl::wait_op_t final : detail::basic_op
{
  error_code   &ec;
  bool          done = false;
  detail::internal_condition_variable var;
  wait_op_t(error_code &ec) : ec(ec) {}

  void complete(error_code ec) override
  {
    done     = true;
    this->ec = ec;
    this->unlink();
    var.notify_all();
  }

  void shutdown() override
  {
    done = true;
    BOOST_SAM_ASSIGN_EC(this->ec, net::error::shut_down);
    this->unlink();
    var.notify_all();
  }

  void wait(lock_type &lock)
  {
    var.wait(lock, [this]{return done;});
  }
};

void condition_variable_impl::wait(error_code &ec)
{
    lock_type lock{mtx_};
    wait_op_t op{ec};
    add_waiter(&op);
    op.wait(lock);

}

} // namespace detail
BOOST_SAM_END_NAMESPACE

#endif // BOOST_SAM_DETAIL_IMPL_CONDITION_VARIABLE_IMPL_IPP
