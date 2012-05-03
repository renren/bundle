#ifndef BASE_BASE_ASYNCALL_H__
#define BASE_BASE_ASYNCALL_H__

#include <boost/detail/atomic_count.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <exception>
#include "base3/pcount.h"

namespace base { namespace detail {

struct Async {
  Async() {}

  static boost::asio::io_service& GetService() {
    static boost::asio::io_service io_;
    return io_;
  }

  template<typename Function>
  static void Post(Function func) {
    WrappedCall<Function> w(func);
    GetService().post(w);
  }
  
  template<typename Function>
  static void Post(Function func, size_t millisec) {
    boost::asio::deadline_timer* timer = new boost::asio::deadline_timer(
      GetService(), boost::posix_time::milliseconds(millisec)
      );

    WrappedDelayCall<Function> w(func, timer);
    w.timer_->async_wait(w);
  }

  template <typename Function>
  static void Post(const Function& func, size_t interval_milliseconds, size_t count) {
    WrappedCountCall<Function> w(func, interval_milliseconds, count);
    Post(w, interval_milliseconds);
  }

  static void Start();
  static void Stop();

private:
  template<typename Function>
  struct WrappedCall {
    void operator()() {
      func();
    }
    WrappedCall(Function func) : func(func) {}
    Function func;
  };

  template<typename Function>
  struct WrappedDelayCall {
    WrappedDelayCall(const Function& func, boost::asio::deadline_timer* timer) 
      : func_(func), timer_(timer)
    {}
    void operator()(const boost::system::error_code& error) {
      if (!error) // expired or != boost::asio::error::operation_aborted
        func_();
    }
    Function func_;
    boost::shared_ptr<boost::asio::deadline_timer> timer_;
  };

  template<typename Function>
  struct WrappedCountCall {
    WrappedCountCall(const Function& f, size_t interval, size_t count)
      : interval_milliseconds_(interval), count_(count)
      , called_(0), func_(f) {}

    void operator()() const {
      ++ called_;
      func_();
      if (0 == count_ || left())
        Post(*this, interval_milliseconds_);
    }

    size_t interval() const {
      return interval_milliseconds_;
    }

    // 还需要运行次数
    size_t left() const {
      // count = 0 表示执行无限次
      return count_ ? (count_ - called_) : 0;
    }

    const size_t interval_milliseconds_;
    const size_t count_;
    mutable size_t called_;     // 已经运行次数
    Function func_;
  };  

private:
  static boost::thread* thread_;
  static boost::asio::io_service::work* work_;
};

} } // namespace base::detail
#endif // BASE_BASE_ASYNCALL_H__
