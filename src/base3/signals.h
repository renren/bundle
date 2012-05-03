#ifndef BASE_BASE_SIGNALS_H__
#define BASE_BASE_SIGNALS_H__

// ----------------------------------------------------
// log, Rotate
#define BASE_SIGNAL_LOG_OPEN     35
#define BASE_SIGNAL_LOG_ROTATE   36

// profile
#define BASE_SIGNAL_PROFILER_START      37
#define BASE_SIGNAL_PROFILER_STOP       38

// ----------------------------------------------------
// config, 本地缓存的map加载，导出
#define BASE_SIGNAL_CONFIG              39

// ----------------------------------------------------
// breakpad
#define BASE_SIGNAL_MINIDUMP           45
#define BASE_SIGNAL_CRASH              46

// ----------------------------------------------------
// cache, 加载/Dump
#define BASE_SIGNAL_CACHE_DUMP         50
#define BASE_SIGNAL_CACHE_DUMP_ROTATE  51
#define BASE_SIGNAL_CACHE_RESTORE      52

// ----------------------------------------------------
// for cwf
#define BASE_SIGNAL_CWF_LOAD_TEMPLATE  60

namespace base {

typedef void (*SignalHandle)(int);

void InstallSignal(int sig, SignalHandle);

}
#endif // BASE_BASE_SIGNALS_H__
