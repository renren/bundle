#ifndef BASE_BASE_COMMON_H__
#define BASE_BASE_COMMON_H__

#include <boost/static_assert.hpp>

//////////////////////////////////////////////////////////////////////
// General Utilities
//////////////////////////////////////////////////////////////////////

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (static_cast<int>((sizeof(x)/sizeof(x[0]))))
#endif // ARRAY_SIZE

/////////////////////////////////////////////////////////////////////////////
// Assertions
/////////////////////////////////////////////////////////////////////////////

#ifndef BASE_NO_DEBUG

namespace base {

// Break causes the debugger to stop executing, or the program to abort
void Break();

// LogAssert writes information about an assertion to the log
void LogAssert(const char * function, const char * file, int line, const char * expression);

inline void Assert(bool result, const char * function, const char * file, int line, const char * expression) {
  if (!result) {
    LogAssert(function, file, line, expression);
    Break();
  }
}

size_t CurrentMemoryUsage();
bool GetMemInfo(size_t *total, size_t *free);
}; // namespace base

#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ ""
#endif

#ifndef ASSERT
#define ASSERT(x) ::base::Assert((x),__FUNCTION__,__FILE__,__LINE__,#x)
#endif

#ifndef VERIFY
#define VERIFY(x) ::base::Assert((x),__FUNCTION__,__FILE__,__LINE__,#x)
#endif

#else // BASE_NO_DEBUG

#ifndef ASSERT
#define ASSERT(x) (void)0
#endif

#ifndef VERIFY
#define VERIFY(x) (void)(x)
#endif

#endif // !ENABLE_DEBUG

#ifndef STATIC_ASSERT
#define STATIC_ASSERT BOOST_STATIC_ASSERT
#endif

namespace base {

void Sleep(int millisec);

}

#endif // BASE_BASE_COMMON_H__
