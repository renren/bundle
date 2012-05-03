#include <fstream>
#include <sstream>

#ifdef OS_LINUX
  #include <unistd.h>       // getpid
  #include <sys/time.h>     // getrusage
  #include <sys/resource.h> // getrusage
#elif defined(OS_WIN)
  #include <process.h>  // getpid
#endif

#include "base3/common.h" // for ARRAY_SIZE
#include "base3/pcount.h"
#include "base3/baseasync.h"
#include "base3/logging.h"

namespace base {

// TODO: how to use getrusage?
void xar_run() {
  xar& x = xar::instance();
  std::ofstream ostem(x.filename().c_str(), std::ios::app);
  x.output(ostem);
}

bool xar::start() {
  xar& x = xar::instance();

  if (x.filename().empty()) {
    // ../log/xar.pid
    std::ostringstream ostem;
    ostem << "../log/xar." << getpid();
    x.set_filename(ostem.str());
  }

  LOG(INFO) << x.filename();

  // TODO: use member deadline_timer, for interval_seconds change
  detail::Async::Post(boost::bind(xar_run), x.interval_seconds() * 1000, 0);
  return true;
}

// rusage
#if 0
struct rusage {
  struct timeval ru_utime; /* user time used */
  struct timeval ru_stime; /* system time used */
  long   ru_maxrss;        /* maximum resident set size */
  long   ru_ixrss;         /* integral shared memory size */
  long   ru_idrss;         /* integral unshared data size */
  long   ru_isrss;         /* integral unshared stack size */
  long   ru_minflt;        /* page reclaims */
  long   ru_majflt;        /* page faults */
  long   ru_nswap;         /* swaps */
  long   ru_inblock;       /* block input operations */
  long   ru_oublock;       /* block output operations */
  long   ru_msgsnd;        /* messages sent */
  long   ru_msgrcv;        /* messages received */
  long   ru_nsignals;      /* signals received */
  long   ru_nvcsw;         /* voluntary context switches */
  long   ru_nivcsw;        /* involuntary context switches */
};

http://www.gnu.org/s/libc/manual/html_node/Resource-Usage.html
#endif

const char* xar::rusage_name[] = {
  "mem",
  "us",
  "sy",
#if 0 // always is 0
  "RES",
  "SHR",   // ixrss
  ".data", // idrss
  "stack",
#endif  

  "page",
  "pfalt",
  "swaps",
  "in",
  
  "out",
  // "msgsnd",
  // "msgrcv",
  "signal",
  "ctxsw",
  "ictxsw"
};
size_t xar::rusage_size = ARRAY_SIZE(rusage_name);
  
#ifdef OS_LINUX
size_t timeval2ms(struct timeval& tv) {
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif

std::string xar::rusage() {
  std::ostringstream ostem;

  ostem << std::setw(pcount::align) << CurrentMemoryUsage() / 1024;
#ifdef OS_LINUX
  struct rusage usage;
  if (0 == getrusage(RUSAGE_SELF, &usage)) {
    ostem
      << std::setw(pcount::align) << timeval2ms(usage.ru_utime)
      << std::setw(pcount::align) << timeval2ms(usage.ru_stime)
#if 0 // always 0
      << std::setw(pcount::align) << usage.ru_maxrss
      << std::setw(pcount::align) << usage.ru_ixrss
      << std::setw(pcount::align) << usage.ru_idrss
      << std::setw(pcount::align) << usage.ru_isrss
#endif

      << std::setw(pcount::align) << usage.ru_minflt
      << std::setw(pcount::align) << usage.ru_majflt
      << std::setw(pcount::align) << usage.ru_nswap
      << std::setw(pcount::align) << usage.ru_inblock
      
      << std::setw(pcount::align) << usage.ru_oublock
      // << std::setw(pcount::align) << usage.ru_msgsnd
      // << std::setw(pcount::align) << usage.ru_msgrcv
      << std::setw(pcount::align) << usage.ru_nsignals
      << std::setw(pcount::align) << usage.ru_nvcsw
      << std::setw(pcount::align) << usage.ru_nivcsw;
  }
#endif
  

  return ostem.str();
}

}
