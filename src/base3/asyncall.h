#ifndef BASE_ASYNCALL_H__
#define BASE_ASYNCALL_H__

#include <boost/detail/atomic_count.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <exception>

#include "base3/pcount.h"

namespace base {

struct AsyncRunner {
  AsyncRunner() : stop_(false)
    , thread_size_(4), max_thread_size_(100)
    , busy_(0), task_(0)
    , check_thread_(0)
  {}

  size_t thread_size() const {
    return thread_size_;
  }
  void set_thread_size(size_t c);
  void set_max_thread_size(size_t c) {
    max_thread_size_ = c;
  }

  static AsyncRunner& instance() {
    static AsyncRunner a_;
    return a_;
  }

  static boost::asio::io_service& GetService() {
    static boost::asio::io_service io_;
    return io_;
  }

  bool Start();
  void Stop();

  // how busy we are
  void BusyInc() {
    ++ busy_;
  }
  void BusyDec() {
    -- busy_;
  }
  size_t busy() const {
    return busy_;
  }

  // task in queue
  size_t task() const {
    return task_ < 0 ? 0 : task_;
  }
  void TaskInc() {
    ++ task_;
  }
  void TaskDec() {
    -- task_;
  }

private:
  void CreateOne();
  void DestroyOne();

  void Check();
  void Run();

private:
  volatile bool stop_;
  size_t thread_size_, max_thread_size_;
  boost::detail::atomic_count busy_;
  boost::detail::atomic_count task_;
  boost::thread_group g_;
  boost::thread* check_thread_;
};

extern XAR_DECLARE(post);

namespace detail {

template<typename Function>
struct WrappedCall {
  void operator()() {
    AsyncRunner::instance().BusyInc();
    
    func();

    AsyncRunner::instance().BusyDec();
    AsyncRunner::instance().TaskDec();
  }
  WrappedCall(Function func) : func(func) {}
  Function func;
};

// template<typename Function>
// void MakeWrappedCall(Function func) {
//   WrappedCall<Function> w(func);
//   AsyncRunner::GetService().post(w);
// }

} // detail

template <typename Function>
void Post(Function func) {
  XAR_INC(post);
  AsyncRunner::instance().TaskInc();

#if 0
  AsyncRunner::GetService().post(func);
#else
  detail::WrappedCall<Function> w(func);
  AsyncRunner::GetService().post(w);
#endif
}

// 所有时间为毫秒，可能带来一些适用性。但是容易忘记 * 1000
template <typename Function>
void Post(const Function& func, size_t interval_milliseconds);

// count = 0 表示执行无限次，每次间隔 interval_milliseconds
template <typename Function>
void Post(const Function& func, size_t interval_milliseconds, size_t count);

// 最大限度保证成功，如果失败重试n次
template<typename Function>
void SurePost(const Function& func, size_t interval_milliseconds, size_t retry);


namespace detail {

// 目的：保存 boost::asio::deadline_timer
//   1 deadline_timer is noncopyable
//   2 deadline_timer析构时会导致timer失效
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

// template<typename Function>
// void MakeDelayCall(const Function& func, boost::asio::deadline_timer* timer) {
//   WrappedDelayCall<Function> w(func, timer);
//   w.timer_->async_wait(w);
// }
} // detail

extern XAR_DECLARE(delaypost);

template <typename Function>
void Post(const Function& func, size_t interval_milliseconds) {
  XAR_INC(delaypost);

  boost::asio::deadline_timer* timer = new boost::asio::deadline_timer(
    AsyncRunner::GetService(), boost::posix_time::milliseconds(interval_milliseconds)
    );

  // detail::MakeDelayCall(func, timer);
  detail::WrappedDelayCall<Function> w(func, timer);
  w.timer_->async_wait(w);
}

namespace detail {

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

} // detail

extern XAR_DECLARE(countpost);

template <typename Function>
void Post(const Function& func, size_t interval_milliseconds, size_t count) {
  XAR_INC(countpost);
  detail::WrappedCountCall<Function> w(func, interval_milliseconds, count);
  Post(w, interval_milliseconds);
}

namespace detail {

template<typename Function>
struct WrappedSureCall {
  WrappedSureCall(const Function& func, size_t interval, size_t retry = 3)
    : retry_(retry), interval_(interval), func_(func) {}

  bool operator()() const {
    -- retry_;

    bool failed = false;
    try {
      func_();
    }
    catch (const std::exception& e) {
      failed = true;
#ifdef LOG
      LOG(WARNING) << e.what(); // TODO: name, proxy
#endif
    } catch(const std::exception* e) {
      failed = true;
#ifdef LOG
      LOG(WARNING) << e->what(); // TODO: name, proxy
#endif
    } catch(...) {
      failed = true;
#ifdef LOG
      LOG(WARNING) << "unknown exception";
#endif
    }

    AsyncRunner::instance().TaskDec();

    return failed ? retry_ > 0 : false;
  }

  mutable size_t retry_;
  const size_t interval_;
  Function func_;
};

// template<typename Function>
// void SureCall(const WrappedSureCall<Function>& call) {
//   if (call())
//     SurePost(call.func_, call.interval_, call.retry_);
// }

} // detail

template<typename Function>
void SurePost(const Function& func, size_t interval_milliseconds, size_t retry) {
  AsyncRunner::instance().TaskInc();
  detail::WrappedSureCall<Function> w(func, interval_milliseconds, retry);
  Post(w, interval_milliseconds); // call delay
}

}
#endif // BASE_ASYNCALL_H__
