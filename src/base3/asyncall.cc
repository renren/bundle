#include "base3/asyncall.h"
#include "base3/pcount.h"
#include "base3/logging.h"

namespace base {

void AsyncRunner::set_thread_size(size_t c) {
  size_t start = g_.size();
  if (c <= start) // TODO: 能缩减线程数么?
    return;

  for (size_t i=start; i<c; ++i)
    CreateOne();

  thread_size_ = g_.size();
}

XAR_IMPL(post);
XAR_IMPL(delaypost);
XAR_IMPL(countpost);

bool AsyncRunner::Start() {
  stop_ = false;
  GetService().reset(); // 如果之前被Stop过, 必须reset
  // TODO: io_service::reset reset io_service::work ?

  static bool once_ = false;
  if (!once_) {
    XAR_POLL("busy", boost::bind(&AsyncRunner::busy, &AsyncRunner::instance()));
    XAR_POLL("task", boost::bind(&AsyncRunner::task, &AsyncRunner::instance()));
    once_ = true;
  }

  for (size_t i=0; i<thread_size_; ++i)
    CreateOne();
 
  check_thread_ = new 
    boost::thread(boost::bind(&AsyncRunner::Check, this));
  return true;
}

void AsyncRunner::Stop() {
  stop_ = true;

  GetService().stop();
  
  // TDOO: use g_.interrupt_all 防止有人霸占线程
  g_.join_all();
  check_thread_->join();
  delete check_thread_;
}

void AsyncRunner::Check() {
  boost::asio::deadline_timer timer(GetService());
  while(!stop_) {
    timer.expires_from_now(boost::posix_time::milliseconds(1000));
    timer.wait();

    // 膨胀
    if (busy_ + 2 > thread_size() && thread_size() < max_thread_size_) {
      LOG(WARNING) << "inflate thread size, current:" << thread_size() 
        << " max: " << max_thread_size_;
      set_thread_size(thread_size() + 1);
    }

    // 缩小
    if (busy_ < thread_size() && thread_size() > 1) {
      // 
    }
  }
}

void AsyncRunner::Run() {
  // 1 启动 timer
  // 2 执行 poll()
  // 3 如果超时,启动Check
}

void AsyncRunner::CreateOne() {
  g_.add_thread(new boost::thread(
      boost::bind(
        &boost::asio::io_service::run, &AsyncRunner::GetService()
        // &AsyncRunner::Run, this
      )
    ));
}

void AsyncRunner::DestroyOne() {
  // TODO:
}

// now start in pcount.cc 
// static bool async_running = AsyncRunner::instance().Start();
}
