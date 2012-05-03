#ifndef BUNDLE_FILELOCK_H__
#define BUNDLE_FILELOCK_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef OS_WIN
#include <unistd.h>
#endif

#include <string>

namespace bundle {

class FileLock {
  public:

    FileLock(const char* name, int timeout=10000, int delay=500) 
      : lockfile_(name), timeout_(timeout), delay_(delay), locked_(false) {
    }

    ~FileLock() {
      if (locked_) {
        Unlock();
      }
    }

    bool Lock() {
      int delay = delay_;
      do {
        locked_ = Lock(lockfile_.c_str());
        if (!locked_) {
          if (delay < timeout_) {
#ifndef OS_WIN
            usleep(delay);
#endif
            delay += delay;
          }
          else {
            break;
          }
        }
      } while(!locked_);
      return locked_;
    }

    bool TryLock() {
      // ASSERT(!locked_);
      locked_ = Lock(lockfile_.c_str());
      return locked_;
    }

    void Unlock() {
      // ASSERT(locked_);
      if (locked_)
        Unlock(lockfile_.c_str());
    }

    bool IsLocked() const {
      return locked_;
    }

    static bool Lock(const char* name) {
#ifndef OS_WIN
      // O_EXCL is the KEY
      int fd = open(name, O_RDWR | O_EXCL | O_CREAT, 
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
      if (-1 == fd )
        return false;

      close(fd);
      return true;
#endif
			return true;
    }

    static bool Unlock(const char* name) {
      return 0 == unlink(name);
    }

private:
  std::string lockfile_;
  bool locked_;
  int delay_;
  // int fd_;
};

}//namespace bundle

#endif // BUNDLE_FILELOCK_H__
