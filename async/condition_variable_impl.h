#ifndef MONEYMAKER_ASYNC_CONDITION_VARIABLE_IMPL_H
#define MONEYMAKER_ASYNC_CONDITION_VARIABLE_IMPL_H

#include <chrono>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>

namespace websocket
{
  struct condition_variable_impl
  {
    condition_variable_impl(boost::asio::any_io_executor exec) : timer_(std::move(exec))
    {
      timer_.expires_at(std::chrono::steady_clock::time_point::max());
    }

    template <typename Predicate>
    boost::asio::awaitable<void> wait(Predicate pred);

    void notify_one();
    void notify_all();

  private:
    boost::asio::steady_timer timer_;
  };

  template <typename Predicate>
  boost::asio::awaitable<void> condition_variable_impl::wait(Predicate pred)
  {
    while (!pred())
    {
      boost::system::error_code ec;
      co_await timer_.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    }
  }
}  // namespace websocket

#endif
