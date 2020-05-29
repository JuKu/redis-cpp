//-------------------------------------------------------------------
//  redis-cpp
//  https://github.com/tdv/redis-cpp
//  Created:     03.2020
//  Copyright (C) 2020 tdv
//-------------------------------------------------------------------

#ifndef REDISCPP_PURE_CORE

// STD
#include <stdexcept>
#include <utility>

// BOOST
#include <boost/asio.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>

namespace rediscpp
{
namespace detail
{

class tcp_stream_device final
{
public:
    using char_type = char;
    using category = boost::iostreams::bidirectional_device_tag;

    tcp_stream_device(boost::asio::ip::tcp::socket &socket)
        : socket_{socket}
    {
    }

    [[nodiscard]]
    std::streamsize read(char *s, std::streamsize n)
    {
        boost::system::error_code ec;

        auto rval = socket_.read_some(boost::asio::buffer(
                s, static_cast<std::size_t>(n)), ec);

        if (!ec)
            return static_cast<std::streamsize>(rval);
        else if (ec == boost::asio::error::eof)
            return static_cast<std::streamsize>(-1);
        else
            throw boost::system::system_error(ec, "read_some");
    }

    [[nodiscard]]
    std::streamsize write(char const *s, std::streamsize n)
    {
        boost::system::error_code ec;
        auto rval = socket_.write_some(boost::asio::buffer(
                s, static_cast<std::size_t>(n)), ec);
        if (!ec)
            return static_cast<std::streamsize>(rval);
        else if (ec == boost::asio::error::eof)
            return static_cast<std::streamsize>(-1);
        else
            throw boost::system::system_error(ec, "write_some");
    }



private:
    boost::asio::ip::tcp::socket& socket_;

};

class stream final
{
public:
    stream(const char* host, int port)
    {
        boost::asio::ip::tcp::resolver resolver{io_context_};

        //resolver.resolve() is deprecated
        /*auto endpoints = resolver.resolve(std::move(host), std::move(port));
        auto iter = std::begin(endpoints);
        if (iter == std::end(endpoints))
            throw std::runtime_error{"There is no any endpoint."};*/

        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

        socket_.connect(endpoint);
        socket_.set_option(boost::asio::ip::tcp::no_delay{});

        stream_ = std::make_unique<stream_type>(socket_);
    }

    [[nodiscard]]
    std::iostream* get_stream()
    {
        return stream_.get();
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_{io_context_};

    using stream_type = boost::iostreams::stream<tcp_stream_device>;
    std::unique_ptr<stream_type> stream_;
};

}   // namespace detail

#ifdef REDISCPP_HEADER_ONLY
inline
#endif  // !REDISCPP_HEADER_ONLY
std::shared_ptr<std::iostream> make_stream(char* host,
                                           int port)
{
    auto stream = std::make_shared<detail::stream>(host, port);
    return std::shared_ptr<std::iostream>{stream, stream->get_stream()};
}

}   // namespace rediscpp

#endif  // !REDISCPP_PURE_CORE
