#include "base3/signals.h"

#include <signal.h>
#include <list>
#include <boost/foreach.hpp>
#include <sstream>

#if defined(OS_LINUX) && defined(USE_PROFILER)
#include <google/profiler.h>
#endif

#include "base3/common.h"
#include "base3/once.h"
#include "base3/startuplist.h"
#include "base3/globalinit.h"

#if defined(OS_LINUX) && defined(USE_BREAKPAD)
// in 3rdparty
extern bool RequestMinidump(const std::string &dump_path);
#endif

namespace base {

void InstallSignal(int sig, SignalHandle h) {
#ifdef OS_LINUX
  signal(sig, h);
#elif defined(OS_WIN)
  // windows不支持自定义 signal
#endif
}

// base: log, rotate
// profile: start / stop
// 

static void SignalProfile(int sig) {
#if defined(OS_LINUX) && defined(USE_PROFILER)
  if (BASE_SIGNAL_PROFILER_START == sig) {
    std::ostringstream ostem;
    ostem << "/tmp/cell_" << getpid() << ".prf";
    ProfilerStart(ostem.str().c_str());
  }
  else if (BASE_SIGNAL_PROFILER_STOP == sig) 
    ProfilerStop();
#endif
}

static void SignalCrash(int sig) {
  ASSERT(BASE_SIGNAL_MINIDUMP == sig);
  if (BASE_SIGNAL_MINIDUMP == sig) {
#if defined(OS_LINUX) && defined(USE_BREAKPAD)
    ::RequestMinidump(".");
#endif
  } 
  else if (BASE_SIGNAL_CRASH == sig) {
    *(char *)0 = 'a';
  }
}

static void AddCommonSignal() {
  InstallSignal(BASE_SIGNAL_PROFILER_START, SignalProfile);
  InstallSignal(BASE_SIGNAL_PROFILER_STOP, SignalProfile);

  InstallSignal(BASE_SIGNAL_MINIDUMP, SignalCrash);
  InstallSignal(BASE_SIGNAL_CRASH, SignalCrash);

  // TODO: log rotate
}

GLOBAL_INIT(COMMON_SIGNALS, {
  AddStartup(&AddCommonSignal);
});

}
