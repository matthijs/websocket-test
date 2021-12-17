#ifndef MONEYMAKER_WEBSOCKET_CONNECTION_H
#define MONEYMAKER_WEBSOCKET_CONNECTION_H

#include <memory>

#include <fmt/core.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include "connection_impl.h"

namespace websocket
{
  struct connection_lifetime
  {
    connection_lifetime(std::shared_ptr<connection_impl> impl)
        : lifetime_(construct_lifetime(impl)), impl_(std::move(impl))
    {
    }

    const std::shared_ptr<connection_impl>& get_impl() const
    {
      return impl_;
    }

  private:
    static std::shared_ptr<void> construct_lifetime(const std::shared_ptr<connection_impl>& impl)
    {
      static int useful_address;
      auto deleter = [impl](void*)
      {
        boost::asio::co_spawn(
            impl->get_executor(),
            [impl]() -> boost::asio::awaitable<void>
            {
              fmt::print("connection_lifetime::deleter\n");
              co_await impl->shutdown();
              fmt::print("connection_lifetime::deleter done\n");
            },
            boost::asio::detached);
      };
      return std::shared_ptr<void>(&useful_address, deleter);
    }

    std::shared_ptr<void> lifetime_;
    std::shared_ptr<connection_impl> impl_;
  };

  struct connection
  {
    connection(connection_lifetime life) : life_(std::move(life))
    {
    }

    void send(std::string_view msg, bool is_text = true);

    boost::asio::awaitable<event> consume();

    // Close the websocket connection and suspend until it is closed.
    void drop(boost::beast::websocket::close_reason cr = boost::beast::websocket::close_code::normal);

    boost::beast::websocket::close_reason reason() const;

  private:
    connection_lifetime life_;
  };

  boost::asio::awaitable<connection> connect(std::string host,
                                             std::string port,
                                             std::string target,
                                             connect_options options = {});
}  // namespace websocket

#endif
