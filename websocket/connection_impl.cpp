#include "connection_impl.h"

namespace websocket
{
  boost::asio::awaitable<void> connection_impl::connect(std::string host,
                                                        std::string port,
                                                        std::string target,
                                                        connect_options opts)
  {
    try
    {
      co_await ws_.connect(get_executor(), std::move(host), std::move(port), std::move(target), std::move(opts));
      transition(state_t::state_running);
    }
    catch (...)
    {
      transition(state_t::state_stopped);
    }
  }

  boost::asio::awaitable<event> connection_impl::consume()
  {
    auto dynbuf = std::make_shared<boost::beast::flat_buffer>();
    auto [ec, len] = co_await ws_.read(*dynbuf);
    if (ec)
    {
      transition(state_t::state_stopped);
      co_return event(ec);
    }

    co_return event(message(dynbuf, ws_.is_binary()));
  }

  boost::asio::awaitable<void> connection_impl::send(std::string frame, bool is_text)
  {
    switch (state_)
    {
      case state_t::state_running:
        co_await send_condition_.wait([this] { return state_ != state_t::state_running || !sending_; });
        if (state_ == state_t::state_running)
        {
          sending_ = true;
          if (is_text)
          {
            ws_.text();
          }
          else
          {
            ws_.binary();
          }

          boost::system::error_code ec;
          co_await ws_.send(boost::asio::buffer(frame), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
          sending_ = false;
          send_condition_.notify_one();
        }
        [[fallthrough]];
      case state_t::state_stopped:
      case state_t::state_initial:
        break;
    }
  }

  boost::asio::awaitable<void> connection_impl::shutdown()
  {
    switch (state_)
    {
      case state_t::state_running:
        co_await ws_.drop();
        co_await state_condition_.wait([this] { return state_ == state_t::state_stopped; });
        [[fallthrough]];
      case state_t::state_stopped:
      case state_t::state_initial:
        break;
    }
  }

  void connection_impl::transition(state_t newstate)
  {
    state_ = newstate;
    state_condition_.notify_all();
    send_condition_.notify_all();
  }
}  // namespace websocket
