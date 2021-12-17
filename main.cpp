#include <fmt/core.h>

#include "websocket/connect_options.h"
#include "websocket/connection.h"

boost::asio::awaitable<void> streaming() {
  std::string host = "streaming.saxobank.com";
  std::string target =
      "/sim/openapi/streamingws/connect?contextId=some-context-id";
  std::string access_token = "token";

  websocket::connect_options opts;
  opts.pingpong_timeout = std::chrono::seconds{0};
  opts.headers.set(boost::beast::http::field::host, host);
  opts.headers.set(boost::beast::http::field::authorization,
                   "Bearer " + access_token);

  // never stop...
  while (true) {
    auto ws = co_await websocket::connect(host, "443", target, opts);
    for (;;) {
      auto ev = co_await ws.consume();
      if (ev.is_error()) {
        if (ev.error() == boost::beast::websocket::error::closed) {
          fmt::print("connection closed: {}\n", ws.reason());
          co_return;
        } else {
          fmt::print("connection error: {}, reconnecting\n",
                     ev.error().message());
          break;
        }
      } else {
        fmt::print("handle message: {}\n", ev.message().text());
      }
    }
  }
}

int main(int argc, char **argv) {
  boost::asio::io_context ioc;
  boost::asio::co_spawn(
      ioc, [] { return streaming(); }, boost::asio::detached);

  ioc.run();
  return 0;
}
