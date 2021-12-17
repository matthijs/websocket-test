#include "event.h"

namespace websocket
{
  event::event(boost::system::error_code ec) : var_(ec)
  {
  }

  event::event(struct message msg) : var_(std::move(msg))
  {
  }

  bool event::is_error() const
  {
    return std::holds_alternative<boost::system::error_code>(var_);
  }

  bool event::is_message() const
  {
    return std::holds_alternative<struct message>(var_);
  }

  const boost::system::error_code& event::error() const
  {
    return std::get<boost::system::error_code>(var_);
  }

  struct message& event::message()
  {
    return std::get<struct message>(var_);
  }

  const struct message& event::message() const
  {
    return std::get<struct message>(var_);
  }
}  // namespace websocket
