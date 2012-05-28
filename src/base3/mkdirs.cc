#include "base3/mkdirs.h"
#include <stdio.h>
#include <errno.h>

#ifdef OS_WIN
  #include <direct.h>
#endif

#ifdef OS_LINUX
  #include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <string>

namespace base {
#ifdef OS_LINUX
// return 0:OK -1:error
// The old implementation is not multi-process safe, now it will try
// to creat the path from the head, and when part of the path exists, it will 
// try to creat the next part.
int mkdirs(const char *pathname, mode_t mode) {
  int retval = 0;
  #define PATH_LENGTH_MAX 300
  char path[PATH_LENGTH_MAX];

  strncpy(path, pathname, PATH_LENGTH_MAX - 1);
	path[PATH_LENGTH_MAX - 1] = '\0';

  size_t path_len = strlen(path);
  if (path_len && '/' == path[path_len - 1])
    path[path_len - 1] = '\0';

  std::string subpath;
  char *pa = path;
  if ('/' == path[0]) {
    subpath.append("/");
    pa++;
  }
  else if (('.' == path[0]) && ('/' == path[1])) {
    subpath.append("./");
    pa+=2;
  }

  char *p = 0;
  while (1) {
   if (0 != (p = strchr(pa, '/'))) {
     *p = '\0';
     subpath.append(pa);
     //mkdir error
     if (0 != (retval = mkdir(subpath.c_str(), mode))) {
       if (EEXIST == errno) {
         p++;
         pa = p;
         subpath.append("/");
         continue;
       }
       //error happen
       else {
         return retval;
       }
     }
     //mkdir OK
     else {
       p++;
       pa = p;
       subpath.append("/");
     }
   }
   //the last chars after '/'
   else {
     subpath.append(pa);
     if (0 != (retval = mkdir(subpath.c_str(), mode))) {
       if (EEXIST == errno) {
         return 0;
       }
       else {
         return retval;
       }
     }
     //mkdir OK
     else {
       return 0;
     }
   }
  }

}

#else

int mkdirs(const char *pathname, int mode) {
  int retval = 0;
#define PATH_LENGTH_MAX 300
  char path[PATH_LENGTH_MAX];

  strncpy(path, pathname, PATH_LENGTH_MAX - 1);
  path[PATH_LENGTH_MAX - 1] = 0;

  size_t path_len = strlen(path);

  // ugly replace
  for (size_t i=0; i<path_len; ++i) {
    if (path[i] == '/')
      path[i] = '\\';
  }
  
  while (path_len && '\\' == path[path_len - 1])
    path[path_len - 1] = 0;

  while (0 != (retval = _mkdir(path))) {
    char subpath[PATH_LENGTH_MAX] = "", *delim;
    if (NULL == (delim = strrchr(path, '\\')))
      return retval;
    strncat(subpath, path, delim - path);     // Appends NUL
    if (0 != mkdirs(subpath, mode))
      break;
  }
  return retval;
}

#endif

}
