#include "connection.h"

namespace websocket
{
  void connection::send(std::string_view msg, bool is_text)
  {
    auto impl = life_.get_impl();

    boost::asio::co_spawn(
        impl->get_executor(),
        [impl, is_text, msg = std::string(msg.begin(), msg.end())]() mutable
        { return impl->send(std::move(msg), is_text); },
        boost::asio::detached);
  }

  boost::asio::awaitable<event> connection::consume()
  {
    auto impl = life_.get_impl();
    if (co_await boost::asio::this_coro::executor == impl->get_executor())
    {
      co_return co_await impl->consume();
    }
    else
    {
      co_return co_await boost::asio::co_spawn(
          impl->get_executor(), [impl] { return impl->consume(); }, boost::asio::use_awaitable);
    }
  }

  void connection::drop(boost::beast::websocket::close_reason cr)
  {
    auto impl = life_.get_impl();

    boost::asio::co_spawn(
        impl->get_executor(), [impl]() mutable { return impl->shutdown(); }, boost::asio::detached);
  }

  boost::beast::websocket::close_reason connection::reason() const
  {
    return life_.get_impl()->reason();
  }

  boost::asio::awaitable<connection> connect(std::string host,
                                             std::string port,
                                             std::string target,
                                             connect_options options)
  {
    auto impl = std::make_shared<connection_impl>(co_await boost::asio::this_coro::executor);
    co_await impl->connect(std::move(host), std::move(port), std::move(target), std::move(options));
    co_return connection(connection_lifetime(impl));
  }
}  // namespace websocket
