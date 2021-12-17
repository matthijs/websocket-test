#ifndef MONEYMAKER_WEBSOCKET_MESSAGE_H
#define MONEYMAKER_WEBSOCKET_MESSAGE_H

#include <memory>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/span.hpp>

namespace websocket
{
  struct message
  {
    message(std::shared_ptr<boost::beast::flat_buffer> data, bool isbin = false);

    bool is_text() const;
    bool is_binary() const;
    std::string_view text() const;
	boost::beast::span<std::byte> binary() const;

  private:
    std::shared_ptr<boost::beast::flat_buffer> data_;
    bool is_binary_{false};
  };
}  // namespace websocket

#endif
