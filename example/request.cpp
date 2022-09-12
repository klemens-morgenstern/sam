// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/asio/coroutine.hpp>
#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <boost/asem/guarded.hpp>
#include <boost/asem/st.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <iostream>
#include <memory>

namespace http = boost::beast::http;

/// a request operation
template<typename RequestBody, typename ResponseBody>
struct request_op : boost::asio::coroutine
{
    boost::asio::ip::tcp::socket & sock;

    http::request<RequestBody> &request;
    http::response<ResponseBody> &response;
    std::unique_ptr<boost::beast::flat_buffer> ptr = std::make_unique<boost::beast::flat_buffer>();

    template<typename Self>
    void operator()(Self && self,
                    boost::system::error_code ec = {},
                    std::size_t = 0u)
    {
        if (ec)
            self.complete(ec);

        reenter(this)
        {
            this->request.prepare_payload();
            yield http::async_write(this->sock, this->request, std::move(self));
            yield http::async_read(this->sock, *ptr, this->response, std::move(self));
            self.complete(ec);
        }
    }
};

// still unsafe to call from multiple threads at once

template<typename RequestBody, typename ResponseBody, typename CompletionToken>
auto async_request(
        boost::asio::ip::tcp::socket & sock,
        http::request<RequestBody> &request,
        http::response<ResponseBody> &response,
        CompletionToken && completion_token)
{
    return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(
            request_op<RequestBody, ResponseBody>{{}, sock, request, response}, completion_token, sock);
}


int main(int argc, const char ** argv)
{
    http::request<http::empty_body> req1{http::verb::get, "index.html",  11},
                                    req2{http::verb::get, "doc/",  11};
    http::response<http::string_body> res1, res2;
    boost::asio::io_context ctx;

    boost::asio::ip::tcp::resolver resolver(ctx);
    boost::asio::ip::tcp::socket stream(ctx);
    auto const results = resolver.resolve("boost.org", "80");
    stream.connect(*results);

    boost::asem::st::mutex mtx{ctx};

    boost::asem::guarded(mtx, [&](auto && token) { return async_request(stream, req1, res1, std::move(token)); }, boost::asio::detached);
    boost::asem::guarded(mtx, [&](auto && token) { return async_request(stream, req2, res2, std::move(token)); }, boost::asio::detached);

    ctx.run();

    std::cout << "Res1 : " << res1 << std::endl;
    std::cout << "Res2 : " << res2 << std::endl;

    return 0;
}