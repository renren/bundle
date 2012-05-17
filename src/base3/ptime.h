#ifndef OCE_BASE_PTIME_H__
#define OCE_BASE_PTIME_H__

#ifdef OS_LINUX
#include <sys/time.h>
#endif

#include <ctime>

namespace base {

#ifdef OS_LINUX
namespace detail {
// in milliseconds
inline double timeval_sub(const struct timeval *t1, const struct timeval *t2) {
  return (double)((t2->tv_sec * 1000000 + t2->tv_usec)
    - (t1->tv_sec * 1000000 + t1->tv_usec))/1000;
}
} // detail
#endif // OS_LINUX

class ptime {
public:
  explicit ptime(const char* name, bool auto_log = true, int threshold = 0)
    : auto_log_(auto_log), name_(name), threshold_(threshold) {
#ifdef OS_LINUX
    gettimeofday(&wall_clock_, 0);
#endif
  }

  explicit ptime(const std::string& name, bool auto_log = true, int threshold = 0)
    : auto_log_(auto_log), name_(name), threshold_(threshold) {
#ifdef OS_LINUX
    gettimeofday(&wall_clock_, 0);
#endif
  }

  virtual ~ptime() {
    check_dump();
  }

  void check(const char* name = 0) {
    check_dump();

    if (name)
      name_ = name;
#ifdef OS_LINUX
    gettimeofday(&wall_clock_, 0);
#endif
  }

  // in milliseconds
  double wall_clock() const {
#ifdef OS_LINUX
    struct timeval end;
    gettimeofday(&end, 0);

    return detail::timeval_sub(&wall_clock_, &end);
#endif
    return 0.0;
  }

protected:
  void check_dump() {
    double diff_wall = wall_clock();

    if (auto_log_ || (threshold_ && threshold_ < diff_wall)) {
      dump(name_, diff_wall);
    }
  }

  void dump(const std::string& name, double wall) {
    std::ostringstream os;
    std::istream::fmtflags old_flags = os.setf(std::istream::fixed,
      std::istream::floatfield);

    std::streamsize old_prec = os.precision(3);
    os << name << " clock: " <<  wall << " ms";
    os.flags(old_flags);
    os.precision(old_prec);

#ifdef LOG
    LOG(INFO) << os.str();
#else
    std::cout << os.str() << std::endl;
#endif
  }

  bool auto_log_;
  std::string name_;
  int threshold_;

#ifdef OS_LINUX
  struct timeval wall_clock_;
#endif
};

}

#ifndef BASE_NO_PTIME
#define PTIME(pt, name, log, threshold) \
  base::ptime pt(name, log, threshold)

#define PTIME_CHECK(pt, name) \
  pt.check(name)

#else // NO_PTIME

#define PTIME(pt, name)
#define PTIME_CHECK(pt, name)

#endif

#endif // OCE_BASE_PTIME_H__

