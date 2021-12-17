#ifndef MONEYMAKER_WEBSOCKET_CONNECT_OPTIONS_H
#define MONEYMAKER_WEBSOCKET_CONNECT_OPTIONS_H

#include <chrono>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http/fields.hpp>

#include "../async/stop_source.h"

namespace websocket
{
  boost::asio::ssl::context& default_ssl_context();

  struct connect_options
  {
    boost::beast::http::fields headers;
    std::chrono::milliseconds pingpong_timeout = std::chrono::seconds{30};
    boost::asio::ssl::context& ctx = default_ssl_context();
    stop_token stop;
  };
}  // namespace websocket

#endif
