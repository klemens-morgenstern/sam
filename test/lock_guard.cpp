// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

#include <boost/asem/mt.hpp>
#include <boost/asem/st.hpp>
#include <boost/asem/guarded.hpp>
#include <chrono>
#include <random>
#include <vector>
#include <boost/asem/lock_guard.hpp>

#if !defined(BOOST_ASEM_STANDALONE)
namespace asio = BOOST_ASEM_ASIO_NAMESPACE;
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
using namespace BOOST_ASEM_ASIO_NAMESPACE;
using namespace BOOST_ASEM_ASIO_NAMESPACE::experimental;

using models = std::tuple<st, mt>;
template<typename T>
using context = typename std::conditional<
        std::is_same<st, T>::value,
        io_context,
        thread_pool
>::type;

inline void run_impl(io_context & ctx)
{
    ctx.run();
}

inline void run_impl(thread_pool & ctx)
{
    ctx.join();
}

static int concurrent = 0;

template<typename Mutex>
struct impl : asio::coroutine
{
    int id;
    using mutex = Mutex;
    mutex & mtx;
    std::shared_ptr<asio::steady_timer> tim{std::make_shared<asio::steady_timer>(mtx.get_executor())};

    impl(int id, bool & active, mutex & mtx) : id(id), mtx(mtx)
    {
    }

    template<typename Self>
    void operator()(Self && self, error_code ec = {}, lock_guard<Mutex> lock = {})
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

template<typename T, typename CompletionToken>
auto async_impl(T & mtx, int i, bool & active,  CompletionToken && completion_token)
{
    return BOOST_ASEM_ASIO_NAMESPACE::async_compose<CompletionToken, void(error_code)>(
            impl<T>(i, active, mtx), completion_token, mtx);
}

template<typename T>
void test_sync(T & mtx, std::vector<int> & order)
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
    context<T> ctx;
    std::vector<int> order;
    typename T::mutex mtx{ctx.get_executor()};
    test_sync<basic_mutex<T>>(mtx, order);
    run_impl(ctx);
}
