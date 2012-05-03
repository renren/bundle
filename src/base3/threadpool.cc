#include "base3/threadpool.h"
#include "base3/messagequeue.h"
#include "base3/logging.h"

#include <boost/thread/tss.hpp>
#include <boost/bind.hpp>

#if defined(OS_LINUX) && !defined(__LP64__)
#include "base3/thread.h"
#define USE_STATCK_SIZED_THREAD
#endif

namespace base {

class Worker : public MessageQueue {
public:
  Worker() : thread_(0) {}
  ~Worker() {
    // boost::~thread not join
    // TODO: joinable check? 
    thread_->join();
    delete thread_;
  }

  bool Start(size_t stack_size) {
#ifdef USE_STATCK_SIZED_THREAD
    thread_ = new thread(boost::bind(&Worker::Process, this), stack_size);
#else
    thread_ = new boost::thread(boost::bind(&Worker::Process, this));
#endif
    return thread_->joinable();
  }

  void Join() {
    return thread_->join();
  }

private:
#ifdef USE_STATCK_SIZED_THREAD
  thread* thread_;
#else
  boost::thread* thread_;
#endif
};

ThreadPool::~ThreadPool() {
  boost::mutex::scoped_lock alock(mutex_);
  for (std::list<Worker*>::iterator it=workers_.begin(),end=workers_.end();
    it!=end; ++it) {
    delete *it;
  }
}

bool ThreadPool::Post(Message* m) {
  MessageQueue* w = GetIdle();
  if (w) {
    w->Post(m);
    ++ message_count_;
  }
  return w != 0;
}

void ThreadPool::Stop() {
  // LOG(VERBOSE) << "";
  for(std::list<Worker*>::iterator it=workers_.begin(),end=workers_.end();
    it!=end;
    ++it)
  {
    (*it)->Close();
  }
}

void ThreadPool::Join() {
  // LOG_F(VERBOSE) << "";
  // TODO: 开始Join后，就应该禁止Create
  // 拷贝一份是否代价太大
  std::list<Worker*> threads;
  {
    boost::mutex::scoped_lock alock(mutex_);
    threads = workers_;
  }

  for(std::list<Worker*>::iterator it=threads.begin(),end=threads.end();
    it!=end;
    ++it) {
    (*it)->Join();
  }
}

void ThreadPool::set_size(size_t size, size_t max_size) {
  // shrink or inflate
  size_t current = workers_.size();
  if (current > size) {
    Shrunk(size);
  }
  else if (current < size) {
    for (size_t i=0; i<size - current; ++i) {
      Create();
    }
  }

  if (0 != max_size)
    max_size_ = max_size;
}

Worker* ThreadPool::Create() {
  Worker* w = new Worker();
  if (!w)
    return 0;

  if (w->Start(stack_size_)) {
    boost::mutex::scoped_lock alock(mutex_);
    workers_.push_back(w);
    
    return w;
  }
  
  delete w;
  return 0;
}

MessageQueue* ThreadPool::GetIdle() {
  // 策略:
  // 1 找到第一个非 busy 的MessageQueue添加
  // 2 如果还可以创建足够的线程，创建，并赋予之
  // 3 找到一个 busy_ 稍小的直接 Post
  boost::mutex::scoped_lock alock(mutex_);

  size_t worker_count = workers_.size();

  size_t c = 0;
  size_t min_busy_ = 0xffff;
  Worker* idle_worker_ = 0; // 最闲的 Worker
  // 1
  while (c++ < worker_count) {
    if (i_ == workers_.end())
      i_ = workers_.begin();

    Worker* mq = *i_++;
    size_t busy = mq->busy();
    if (0 == busy) {
      return mq;
    }

    if (busy < min_busy_) {
      min_busy_ = busy;
      idle_worker_ = mq;
    }
  }

  alock.unlock();

  // 2
  if (worker_count < max_size_) {
    Worker* w = Create();
    LOG(INFO) << "ThreadPool inflate to " << c;
    return w;
  }

  // 3
  if (idle_worker_)
    return idle_worker_;

  // TODO: 不返回 first，返回一个随机 Worker
  LOG(INFO) << "ThreadPool all busy, return first: " << worker_count;
  alock.lock();
  i_ = workers_.begin();
  return *i_;
}

void ThreadPool::Shrunk(size_t size) {
  boost::mutex::scoped_lock alock(mutex_);
  for (std::list<Worker*>::iterator it=workers_.begin(); it!=workers_.end(); ) {
    Worker* w = *it;
    if (0 == w->size()) {
      it = workers_.erase(it);
      w->Close();
      delete w;

      if (-- size == 0)
        return;
    }
    else
      ++ it;
  }
}

}
