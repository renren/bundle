#ifndef BASE_BASE_MESSAGEQUEUE_H__
#define BASE_BASE_MESSAGEQUEUE_H__

#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace base {

class Message {
public:
  virtual ~Message() {}
  virtual void Run() {}
};

class MessageQueue {
public:
  MessageQueue() : queue_size_(0), busy_(0), quit_(false), processed_(0) {}
  virtual ~MessageQueue() {}
  void Post(Message* m) {
    {
      boost::mutex::scoped_lock alock(mutex_);
      message_queue_.push_back(m);
      ++ queue_size_;
    }
    queue_not_empty_.notify_all();
  }

  // usage:
  // std::vector<arch::Message*> arr
  // Post(arr.begin(), arr.end());
  template<class Iterator>
  int Post(Iterator beg, Iterator end) {
    int c = 0;
    {
      boost::mutex::scoped_lock alock(mutex_);
      while (beg != end) {
        ++ c;
        message_queue_.push_back(*beg++);
        ++ queue_size_;
      }
    }
    queue_not_empty_.notify_all();
    return c;
  }

  void Close() {
    quit_ = true;
    queue_not_empty_.notify_all();
  }

  void Process() {
    while (!quit_) {
      {
        boost::mutex::scoped_lock alock(mutex_);
        if (message_queue_.empty())
          queue_not_empty_.wait(alock);
      }

      ProcessAll();
    }
  }

  // How busy
  size_t busy() const { return busy_; }

  // process how many Messages
  size_t processed() const { return processed_; }
  size_t size() const { return queue_size_; }

protected:
  void ProcessAll() {
    boost::mutex::scoped_lock alock(mutex_);
    busy_ = queue_size_;

    while (!message_queue_.empty()) {
      Message* p = message_queue_.front();
      message_queue_.pop_front();
      -- queue_size_;

      alock.unlock();

      p->Run();
      delete p;

      ++ processed_;

      alock.lock();
    }

    busy_ = queue_size_;
  }

protected:
  // 使用 list 的原因：其他容器在扩大到一定层度后缩减后不释放内存
  std::list<Message*> message_queue_;
  size_t queue_size_;
  boost::mutex mutex_;
  boost::condition queue_not_empty_;
  volatile size_t busy_;
  volatile bool quit_;
  size_t processed_;
};

} // namespace base
#endif // BASE_BASE_MESSAGEQUEUE_H__
