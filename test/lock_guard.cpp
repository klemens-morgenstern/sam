// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

#include <boost/asem/mutex.hpp>
#include <boost/asem/guarded.hpp>
#include <chrono>
#include <random>
#include <vector>
#include <boost/asem/lock_guard.hpp>

#if !defined(BOOST_ASEM_STANDALONE)
namespace asio = boost::asio;
#include <boost/asio.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/yield.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#else
#include <asio.hpp>
#include <asio/compose.hpp>
#include <asio/yield.hpp>
#include <asio/experimental/parallel_group.hpp>

#endif

using namespace BOOST_ASEM_NAMESPACE;
using namespace net;
using namespace net::experimental;

using models = std::tuple<net::io_context, net::thread_pool>;

inline void run_impl(io_context & ctx)
{
    ctx.run();
}

inline void run_impl(thread_pool & ctx)
{
    ctx.join();
}

static int concurrent = 0;

struct impl : asio::coroutine
{
    int id;
    mutex & mtx;
    std::shared_ptr<asio::steady_timer> tim{std::make_shared<asio::steady_timer>(mtx.get_executor())};

    impl(int id, bool & active, mutex & mtx) : id(id), mtx(mtx)
    {
    }

    template<typename Self>
    void operator()(Self && self, error_code ec = {}, lock_guard<mutex> lock = {})
    {
        reenter(this)
        {
            printf("Entered %d\n", id);
            yield async_lock(this->mtx, std::move(self));
            concurrent ++;
            BOOST_CHECK_EQUAL(concurrent, 1);
            printf("Acquired lock %d\n", id);
            tim->expires_after(std::chrono::milliseconds{10});
            yield tim->async_wait(asio::append(std::move(self), std::move(lock)));
            concurrent --;
            BOOST_CHECK_EQUAL(concurrent, 0);
            BOOST_CHECK(!ec);
            printf("Exited %d %d\n", id, ec.value());
            self.complete(ec);
        }
    }
};

template<typename CompletionToken>
auto async_impl(mutex & mtx, int i, bool & active,  CompletionToken && completion_token)
{
    return net::async_compose<CompletionToken, void(error_code)>(
            impl(i, active, mtx), completion_token, mtx);
}

void test_sync(mutex & mtx, std::vector<int> & order)
{
    bool active = false;

    static int i = 0;
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);
    async_impl(mtx, i ++, active, asio::detached);

}

BOOST_AUTO_TEST_CASE_TEMPLATE(lock_guard_t, T, models)
{
    T ctx;
    std::vector<int> order;
    mutex mtx{ctx.get_executor()};
    test_sync(mtx, order);
    run_impl(ctx);
}


struct impl_t : asio::coroutine
{
    mutex & mtx;

    impl_t(mutex & mtx) : mtx(mtx)
    {
    }

    template<typename Self>
    void operator()(Self && self, error_code ec = {}, lock_guard<mutex> lock = {})
    {
        reenter(this)
        {
            yield async_lock(this->mtx, std::move(self)); BOOST_CHECK(!ec);
            yield async_lock(this->mtx, std::move(self)); BOOST_CHECK(!ec);
            yield async_lock(this->mtx, std::move(self)); BOOST_CHECK(!ec);
            yield async_lock(this->mtx, std::move(self)); BOOST_CHECK(!ec);
            yield async_lock(this->mtx, std::move(self)); BOOST_CHECK(!ec);
            self.complete(ec);
        }
    }
};

template<typename CompletionToken>
auto async_impl(mutex & mtx, CompletionToken && completion_token)
{
    return net::async_compose<CompletionToken, void(error_code)>(
            impl_t(mtx), completion_token, mtx);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(lock_series_t, T, models)
{
    T ctx;
    mutex mtx{ctx.get_executor()};
    bool called = false;
    async_impl(mtx, [&](error_code ec) {BOOST_CHECK(!ec); called = true;});
    run_impl(ctx);
    mtx.lock();
    BOOST_CHECK(called);
}
