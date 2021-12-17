#include "variant_websocket.h"

#include <fmt/core.h>

#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>

namespace websocket
{
  namespace
  {
    template <typename... Ts>
    struct overloaded : Ts...
    {
      using Ts::operator()...;
    };
    template <typename... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    void set_sni(tls_layer& stream, const std::string& host)
    {
      if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
      {
        throw std::system_error(
            boost::system::error_code(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()));
      }
    }

    boost::asio::awaitable<boost::asio::ip::tcp::resolver::results_type> resolve(const std::string& host,
                                                                                 const std::string& port,
                                                                                 stop_token stop)
    {
      fmt::print("weboscket::ns::resolve host: {}, port: {}\n", host, port);
      auto resolver = boost::asio::ip::tcp::resolver(co_await boost::asio::this_coro::executor);
      auto stopconn = stop.connect([&] { resolver.cancel(); });
      co_return co_await resolver.async_resolve(host, port, boost::asio::use_awaitable);
    }

    boost::asio::awaitable<void> connect_tcp(boost::beast::tcp_stream& stream,
                                             boost::asio::ip::tcp::resolver::results_type results,
                                             stop_token stop)
    {
      fmt::print("weboscket::ns::connect_tcp\n");
      stream.expires_after(std::chrono::seconds{30});
      auto stopconn = stop.connect([&] { stream.cancel(); });
      co_await stream.async_connect(results, boost::asio::use_awaitable);
      fmt::print("weboscket::ns::connect_tcp done\n");
    }

    boost::asio::awaitable<void> connect_tls(boost::beast::ssl_stream<boost::beast::tcp_stream>& stream,
                                             const std::string& host,
                                             stop_token stop)
    {
      fmt::print("weboscket::ns::connect_tls\n");
      set_sni(stream, host);
      stream.next_layer().expires_after(std::chrono::seconds{30});
      auto stopconn = stop.connect([&] { stream.next_layer().cancel(); });
      co_await stream.async_handshake(boost::asio::ssl::stream_base::client, boost::asio::use_awaitable);
      fmt::print("weboscket::ns::connect_tls done\n");
    }
  }  // namespace

  auto variant_websocket::connect(boost::asio::any_io_executor exec,
                                  const std::string& host,
                                  const std::string& port,
                                  const std::string& target,
                                  const connect_options& opts) -> boost::asio::awaitable<void>
  {
    auto to = boost::beast::websocket::stream_base::timeout{
        .handshake_timeout = std::chrono::seconds{30},
        .idle_timeout =
            opts.pingpong_timeout.count() ? opts.pingpong_timeout / 2 : boost::beast::websocket::stream_base::none(),
        .keep_alive_pings = opts.pingpong_timeout.count() ? true : false};

    // Only support TLS links
    auto transport = transport_t::transport_tls;
    switch (transport)
    {
      case transport_t::transport_tcp:
        emplace_tcp(exec, to);
        break;
      case transport_t::transport_tls:
        emplace_tls(exec, opts.ctx, to);
        break;
    }

    // Connect tcp
    fmt::print("weboscket::connect: connect to tcp\n");
    auto& tcp_layer = get_tcp();
    co_await connect_tcp(tcp_layer, co_await resolve(host, port, opts.stop), opts.stop);
    fmt::print("weboscket::connect: connect to tcp done\n");

    // Handle TLS
    if (auto tls = query_tls())
    {
      fmt::print("weboscket::connect: setup tls\n");
      co_await connect_tls(*tls, host, opts.stop);
      fmt::print("weboscket::connect: setup tls done\n");
    }

    // Websocket handshake
    set_headers(opts.headers);
    boost::beast::websocket::response_type response;
    fmt::print("weboscket::connect: websocket handshake\n");
    co_await client_handshake(response, host, target);
    fmt::print("weboscket::connect: websocket handshake done\n");

    // Disable timeouts on the tcp layer
    fmt::print("weboscket::connect: never expire\n");
    tcp_layer.expires_never();
    fmt::print("weboscket::connect: never expire done\n");
  }

  tcp_layer& variant_websocket::get_tcp()
  {
    return visit(overloaded{[](ws_layer& ws) -> decltype(auto) { return ws.next_layer(); },
                            [](wss_layer& wss) -> decltype(auto) { return wss.next_layer().next_layer(); }});
  }

  void variant_websocket::set_headers(const boost::beast::http::fields& headers)
  {
    visit(
        [&](auto& ws)
        {
          ws.set_option(boost::beast::websocket::stream_base::decorator(
              [headers](boost::beast::websocket::request_type& req)
              {
                for (auto&& field : headers)
                {
                  req.insert(field.name(), field.value());
                }
              }));
        });
  }

  boost::asio::awaitable<void> variant_websocket::client_handshake(boost::beast::websocket::response_type& response,
                                                                   const std::string& host,
                                                                   const std::string& target)
  {
    fmt::print("variant_websocket::client_handshake: {}, {}\n", host, target);
    return visit([&](auto& ws) { return ws.async_handshake(response, host, target, boost::asio::use_awaitable); });
  }

  boost::asio::any_io_executor variant_websocket::get_executor()
  {
    return visit([](auto& ws) { return ws.get_executor(); });
  }

  boost::asio::awaitable<void> variant_websocket::send_close(boost::beast::websocket::close_reason cr)
  {
    boost::system::error_code ec;
    co_await visit([&](auto& ws)
                   { return ws.async_close(cr, boost::asio::redirect_error(boost::asio::use_awaitable, ec)); });
  }

  boost::asio::awaitable<void> variant_websocket::drop()
  {
    fmt::print("weboscket::drop: send close\n");
    co_await send_close();
    fmt::print("weboscket::drop: send close done\n");
  }

  void variant_websocket::text()
  {
    visit([](auto& ws) { ws.text(); });
  }

  void variant_websocket::binary()
  {
    visit([](auto& ws) { ws.binary(); });
  }

  tls_layer* variant_websocket::query_tls()
  {
    if (std::holds_alternative<wss_layer>(var_))
    {
      return std::addressof(std::get<wss_layer>(var_).next_layer());
    }

    return nullptr;
  }

  void variant_websocket::emplace_tcp(boost::asio::any_io_executor exec,
                                      boost::beast::websocket::stream_base::timeout to)
  {
    auto& ws = var_.emplace<ws_layer>(std::move(exec));
    ws.set_option(to);
  }

  void variant_websocket::emplace_tls(boost::asio::any_io_executor exec,
                                      boost::asio::ssl::context& ctx,
                                      boost::beast::websocket::stream_base::timeout to)
  {
    auto& wss = var_.emplace<wss_layer>(std::move(exec), ctx);
    wss.set_option(to);
  }

  boost::beast::websocket::close_reason variant_websocket::reason() const
  {
    return visit([](auto& ws) { return ws.reason(); });
  }

  boost::asio::awaitable<std::tuple<boost::system::error_code, std::size_t>> variant_websocket::read(
      boost::beast::flat_buffer& buf)
  {
    boost::system::error_code ec;
    auto size = co_await visit(
        [&](auto& ws) { return ws.async_read(buf, boost::asio::redirect_error(boost::asio::use_awaitable, ec)); });

    co_return std::make_tuple(ec, size);
  }

  bool variant_websocket::is_binary() const
  {
    return visit([](auto& ws) { return ws.binary(); });
  }
}  // namespace websocket
