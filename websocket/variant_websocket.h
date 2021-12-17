#ifndef MONEYMAKER_WEBSOCKET_VARIANT_WEBSOCKET_H
#define MONEYMAKER_WEBSOCKET_VARIANT_WEBSOCKET_H

#include <variant>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "connect_options.h"

namespace websocket
{
  enum class transport_t
  {
    transport_tcp,
    transport_tls
  };

  using tcp_layer = boost::beast::tcp_stream;
  using tls_layer = boost::beast::ssl_stream<tcp_layer>;

  struct variant_websocket
  {
    variant_websocket() = default;

    boost::asio::awaitable<void> connect(boost::asio::any_io_executor exec,
                                         const std::string& host,
                                         const std::string& port,
                                         const std::string& target,
                                         const connect_options& opts);

    [[nodiscard]] boost::asio::any_io_executor get_executor();
    [[nodiscard]] boost::asio::awaitable<void> drop();

    void text();
    void binary();
    bool is_binary() const;
    boost::asio::awaitable<std::tuple<boost::system::error_code, std::size_t>> read(boost::beast::flat_buffer& buf);

    template <typename CompletionHandler>
    auto send(boost::asio::const_buffer buf, CompletionHandler&& token);

    boost::beast::websocket::close_reason reason() const;

  private:
    void emplace_tcp(boost::asio::any_io_executor exec, boost::beast::websocket::stream_base::timeout to);
    void emplace_tls(boost::asio::any_io_executor exec,
                     boost::asio::ssl::context& ctx,
                     boost::beast::websocket::stream_base::timeout to);
    tcp_layer& get_tcp();
    tls_layer& get_tls();
    tls_layer* query_tls();

    void set_headers(const boost::beast::http::fields& headers);
    [[nodiscard]] boost::asio::awaitable<void> client_handshake(boost::beast::websocket::response_type& response,
                                                                const std::string& host,
                                                                const std::string& target);
    [[nodiscard]] boost::asio::awaitable<void> send_close(
        boost::beast::websocket::close_reason cr = boost::beast::websocket::close_code::going_away);

    template <typename F>
    auto visit(F&& f) -> decltype(auto);
    template <typename F>
    auto visit(F&& f) const -> decltype(auto);

    using ws_layer = boost::beast::websocket::stream<tcp_layer>;
    using wss_layer = boost::beast::websocket::stream<tls_layer>;
    using variant_t = std::variant<std::monostate, ws_layer, wss_layer>;
    variant_t var_;
  };

  template <typename F>
  auto variant_websocket::visit(F&& f) -> decltype(auto)
  {
    if (auto& var = var_; std::holds_alternative<ws_layer>(var))
    {
      return f(std::get<ws_layer>(var));
    }
    else if (std::holds_alternative<wss_layer>(var))
    {
      return f(std::get<wss_layer>(var));
    }

    throw std::logic_error("invalid websocket visit");
  }

  template <typename F>
  auto variant_websocket::visit(F&& f) const -> decltype(auto)
  {
    if (auto& var = var_; std::holds_alternative<ws_layer>(var))
    {
      return f(std::get<ws_layer>(var));
    }
    else if (std::holds_alternative<wss_layer>(var))
    {
      return f(std::get<wss_layer>(var));
    }

    throw std::logic_error("invalid websocket visit");
  }

  template <typename CompletionHandler>
  auto variant_websocket::send(boost::asio::const_buffer buf, CompletionHandler&& token)
  {
    return visit([&](auto& ws) { return ws.async_write(buf, std::forward<CompletionHandler>(token)); });
  }
}  // namespace websocket

#endif
