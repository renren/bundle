#ifndef BASE_BASE_MKDIRS_H_
#define BASE_BASE_MKDIRS_H_

#include <sys/stat.h>

namespace base {
#ifdef OS_LINUX

int mkdirs(const char *pathname, mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
int mkdirs_old(const char *pathname, mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

#else

int mkdirs(const char *pathname, int mode = 0);

#endif
}
#endif // BASE_BASE_MKDIRS_H_
