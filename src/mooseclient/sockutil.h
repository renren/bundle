#ifndef MOOSECLIENT_SOCKUTIL_H__
#define MOOSECLIENT_SOCKUTIL_H__

#include <sys/types.h>

#ifdef WIN32
  #include <winsock2.h>
  #include <boost/cstdint.hpp>

  // unistd.h
  #define read(a, b, c) recv(a, (char *)b, c, 0)
  #define write(a, b, c) send(a, (const char *)b, c, 0)
  int close(int fd);

  typedef uint32_t socklen_t;
  typedef uint32_t mode_t;
  typedef __int64 ssize_t;

  // sys/time.h
  #if 0
    struct timeval {
      uint64_t tv_sec;
      uint64_t tv_usec;
    };
  #endif

  struct timezone {
    int     tz_minuteswest; /* minutes west of Greenwich */
    int     tz_dsttime;     /* type of dst correction */
  };

  int gettimeofday(struct timeval *tv, struct timezone *tz);

  // 
  #ifndef SIGTRAP
    #define SIGTRAP 11
  #endif
#endif


int parse_sockaddr_port(const char *ip_as_string, struct sockaddr *out, int *outlen);

#endif // MOOSECLIENT_SOCKUTIL_H__
