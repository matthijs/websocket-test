#include "message.h"

namespace websocket
{
  message::message(std::shared_ptr<boost::beast::flat_buffer> data, bool isbin)
      : data_(std::move(data)), is_binary_(isbin)
  {
  }

  bool message::is_text() const
  {
    return !is_binary_;
  }

  bool message::is_binary() const
  {
    return is_binary_;
  }

  std::string_view message::text() const
  {
    auto d = data_->data();
    return std::string_view(reinterpret_cast<const char*>(d.data()), d.size());
  }

  boost::beast::span<std::byte> message::binary() const
  {
    auto d = data_->data();
    return boost::beast::span<std::byte>(reinterpret_cast<std::byte*>(d.data()), d.size());
  }
}  // namespace websocket
