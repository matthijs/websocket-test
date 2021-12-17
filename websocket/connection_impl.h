#ifndef MONEYMAKER_WEBSOCKET_CONNECTION_IMPL_H
#define MONEYMAKER_WEBSOCKET_CONNECTION_IMPL_H

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/beast/websocket/rfc6455.hpp>

#include "connect_options.h"
#include "event.h"
#include "variant_websocket.h"
#include "../async/condition_variable_impl.h"

namespace websocket
{
  struct connection_impl
  {
    connection_impl(boost::asio::any_io_executor exec) : exec_(std::move(exec))
    {
    }

    boost::asio::awaitable<void> connect(std::string host, std::string port, std::string target, connect_options opts);

    boost::asio::awaitable<event> consume();

    boost::asio::awaitable<void> send(std::string frame, bool is_text);

    const boost::asio::any_io_executor& get_executor() const
    {
      return exec_;
    }

    boost::asio::awaitable<void> shutdown();

    boost::beast::websocket::close_reason reason() const
    {
      return ws_.reason();
    }

  private:
    enum class state_t
    {
      state_initial,
      state_running,
      state_stopped
    };

    void transition(state_t newstate);

    boost::asio::any_io_executor exec_;
    variant_websocket ws_;
    condition_variable_impl state_condition_{get_executor()};
    state_t state_{state_t::state_initial};
    condition_variable_impl send_condition_{get_executor()};
    bool sending_{false};
  };
}  // namespace websocket

#endif
