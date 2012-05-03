#ifdef WIN32
#include <windows.h>
#elif defined(POSIX)
#include <dlfcn.h>
#endif

#include "base3/common.h"
#include "cwf/dynalib.h"

namespace cwf {

#ifdef WIN32

bool DynamicLibrary::Load() {
  // TODO: use LoadLibraryW
  handle_ = LoadLibraryA(filename_.c_str());
  return (0 != handle_);
}

void DynamicLibrary::Close() {
  if (handle_) {
    BOOL f = FreeLibrary((HMODULE)handle_);
    ASSERT(f);
    handle_ = 0;
  }
}

void* DynamicLibrary::GetProc(const char* name) {
  if (handle_) {
    return GetProcAddress((HMODULE)handle_, name);
  }
  return 0;
}

#elif defined(POSIX)

bool DynamicLibrary::Load() {
  handle_ = dlopen(filename_.c_str(), RTLD_LAZY);
  return (0 != handle_);
}

void DynamicLibrary::Close() {
  if (handle_) {
    int ret = dlclose(handle_);
    ASSERT(ret == 0);
    handle_ = 0;
  }
}

void* DynamicLibrary::GetProc(const char* name) {
  if (handle_) {
    return dlsym(handle_, name);
  }
  return 0;
}

#endif // POSIX

}
