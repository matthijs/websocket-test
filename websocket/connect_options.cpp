#include "connect_options.h"

namespace websocket
{
  boost::asio::ssl::context& default_ssl_context()
  {
    struct X
    {
      boost::asio::ssl::context ctx;
      X() : ctx(boost::asio::ssl::context::tls_client)
      {
      }
    };

    static X impl;
    return impl.ctx;
  }
}  // namespace websocket
