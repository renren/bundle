#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <fstream>

#ifdef OS_WIN
#include <windows.h>
#endif  // OS_WIN

#include "base3/common.h"
#include "base3/logging.h"

#ifndef BASE_DISABLE_POOL
#include "base3/asyncall.h"
#include "base3/baseasync.h"
#endif

#if defined(OS_LINUX) && defined(USE_TCMALLOC)
#include <unistd.h>
#include <google/malloc_extension.h>
#endif

//////////////////////////////////////////////////////////////////////
// Assertions
//////////////////////////////////////////////////////////////////////

namespace base {

void OutputTrace() {
  // TODO:
}

void Break() {
#ifdef OS_WIN
  ::DebugBreak();
#else // !OS_WIN

#if _DEBUG_HAVE_BACKTRACE
  OutputTrace();
#endif

  // 服务不能因为一个断言而崩溃
  // abort();
#endif // !OS_WIN
}

void LogAssert(const char * function, const char * file, int line, const char * expression) {
  // TODO - if we put hooks in here, we can do a lot fancier logging
  // fprintf(stderr, "%s(%d): %s @ %s\n", file, line, expression, function);
  LOG(WARNING) << file << "(" << line << "): " << expression
    << " @ " << function;
}

#if defined(OS_LINUX) && defined(USE_TCMALLOC)
size_t CurrentMemoryUsage() {
  size_t result;
  if (MallocExtension::instance()->GetNumericProperty(
        "generic.current_allocated_bytes"
        , &result)) {
    return result;
  } else {
    return 0;
  }
}
#else
size_t CurrentMemoryUsage() {
  return 0;
}
#endif

void Sleep(int millisec) {
#ifdef OS_LINUX
  usleep(millisec * 1000);
#else
  ::Sleep(millisec * 1000);
#endif
}

// k unit
bool GetMemInfo(size_t *total, size_t *free) {

  std::ifstream is("/proc/meminfo");
  if (!is.is_open())
    return false;
  char buf[1024];
  is.getline(buf, 1024);
  if (total)
    *total = atoi(buf+12);

  is.getline(buf, 1024);
  int free0 = atoi(buf+12);

  is.getline(buf, 1024);
  int buffer = atoi(buf+12);

  is.getline(buf, 1024);
  int cached = atoi(buf+12);
  is.close();

  if (free)
    *free = free0 + buffer + cached;
  return true;
}

} // namespace base
