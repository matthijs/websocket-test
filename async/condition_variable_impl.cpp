#include "condition_variable_impl.h"

namespace websocket
{
  void condition_variable_impl::notify_one()
  {
    timer_.cancel_one();
  }

  void condition_variable_impl::notify_all()
  {
    timer_.cancel();
  }
}  // namespace websocket
