#ifndef BASE_BASE_THREADPOOL_H__
#define BASE_BASE_THREADPOOL_H__

#include <list>
#include <boost/thread/thread.hpp>

#include "base3/messagequeue.h"

namespace base {

class Worker;

class ThreadPool {
public:
  ThreadPool(size_t init_size, size_t max_size, size_t stack_size = 2 * 1024 * 1024) 
    : init_size_(init_size)
    , max_size_(max_size)
    , stack_size_(stack_size)
    , message_count_(0) {
    set_size(init_size);
    i_ = workers_.begin();
  }

  virtual ~ThreadPool();
  
  bool Post(Message* m);

  // 顺序执行
  template<typename T>
  bool Post(T beg, T end) {
    MessageQueue* w = GetIdle();
    if (w)
      message_count_ += w->Post(beg, end);
    return w != 0;
  }

  void Stop();
  void Join();

  void set_size(size_t size, size_t max_size = 0);
  size_t size() const {
    boost::lock_guard<boost::mutex> guard(mutex_);
    return workers_.size(); 
  }
  size_t message_count() const { return message_count_; }

protected:
  Worker* Create();
  virtual MessageQueue* GetIdle();

  void Shrunk(size_t size);

protected:
  size_t init_size_;
  size_t max_size_;
  size_t stack_size_;

  int message_count_;

  std::list<Worker*> workers_;
  std::list<Worker*>::const_iterator i_;
  mutable boost::mutex mutex_; // guard the list
};

}

#endif // BASE_BASE_THREADPOOL_H__

