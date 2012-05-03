#ifndef BASE_BASE_RWLOCK_H__
#define BASE_BASE_RWLOCK_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <base3/common.h>

namespace base {

// 写优先的读写锁

class ReadWriteLock {
public:
  ReadWriteLock() : readers_(0)
    , writers_(0)
    , read_waiters_(0)
    , write_waiters_(0)
  {}

  void ReadLock() {
    boost::unique_lock<boost::mutex> lock(lock_);
    if (writers_ || write_waiters_) {
      ++ read_waiters_;
      do {
        cond_read_.wait(lock);
      } while(writers_ || write_waiters_);

      -- read_waiters_;
    }
    ++ readers_;
  }

  void ReadUnlock() {
    boost::unique_lock<boost::mutex> lock(lock_);
    -- readers_;
    ASSERT(readers_ >= 0);
    if (write_waiters_)
      cond_write_.notify_one();
  }

  void WriteLock() {
    boost::unique_lock<boost::mutex> lock(lock_);
    if (readers_ || writers_) {
      ++ write_waiters_;
      do {
        cond_write_.wait(lock);
      } while (readers_ || writers_);

      -- write_waiters_;
    }
    ASSERT(writers_ == 0);
    writers_ = 1;
  }

  void WriteUnlock() {
    boost::unique_lock<boost::mutex> lock(lock_);
    ASSERT(writers_ == 1);
    writers_ = 0;
    if (write_waiters_)
      cond_write_.notify_one();
    else if (read_waiters_)
      cond_read_.notify_all();
  }

private:
  boost::mutex lock_;
  boost::condition_variable cond_read_, cond_write_;

  unsigned int readers_, writers_, read_waiters_, write_waiters_;
};

}
#endif // BASE_BASE_RWLOCK_H__
