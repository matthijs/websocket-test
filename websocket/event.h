#ifndef MONEYMAKER_WEBSOCKET_EVENT_H
#define MONEYMAKER_WEBSOCKET_EVENT_H

#include <variant>
#include <boost/system/error_code.hpp>

#include "message.h"

namespace websocket
{
  struct event
  {
    explicit event(boost::system::error_code ec = boost::system::error_code());
    explicit event(struct message msg);

    bool is_error() const;
    bool is_message() const;

    const boost::system::error_code& error() const;
    struct message& message();
    const struct message& message() const;

    using var_t = std::variant<boost::system::error_code, struct message>;
    var_t var_;
  };
}  // namespace websocket

#endif
