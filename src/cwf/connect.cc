#include <sys/types.h>
// #include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include "base3/common.h"
#include "cwf/connect.h"

#define FCGI_LISTENSOCK_FILENO 0

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

namespace cwf {

#define UNIX_PATH_LEN 108

typedef unsigned short int sa_family_t;

struct sockaddr_un {
  sa_family_t	sun_family;              /* address family AF_LOCAL/AF_UNIX */
  char		sun_path[UNIX_PATH_LEN]; /* 108 bytes of socket address     */
};

  /* Evaluates the actual length of `sockaddr_un' structure. */
#define SUN_LEN(p) ((size_t)(((struct sockaddr_un *) NULL)->sun_path) \
  + strlen ((p)->sun_path))

int Connection(const char *addr, unsigned short port, const char *unixsocket) {
  SOCKET fcgi_fd;
  int socket_type, rc = 0;

  struct sockaddr_un fcgi_addr_un;
  struct sockaddr_in fcgi_addr_in;
  struct sockaddr *fcgi_addr;

  socklen_t servlen;

  WORD  wVersion;
  WSADATA wsaData;

  if (unixsocket) {
    memset(&fcgi_addr, 0, sizeof(fcgi_addr));

    fcgi_addr_un.sun_family = AF_UNIX;
    strncpy(fcgi_addr_un.sun_path, unixsocket, UNIX_PATH_LEN);

#ifdef SUN_LEN
    servlen = SUN_LEN(&fcgi_addr_un);
#else
    /* stevens says: */
    servlen = strlen(fcgi_addr_un.sun_path) + sizeof(fcgi_addr_un.sun_family);
#endif
    socket_type = AF_UNIX;
    fcgi_addr = (struct sockaddr *) &fcgi_addr_un;
  } else {
    fcgi_addr_in.sin_family = AF_INET;
    if (addr != NULL) {
      fcgi_addr_in.sin_addr.s_addr = inet_addr(addr);
    } else {
      fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    fcgi_addr_in.sin_port = htons(port);
    servlen = sizeof(fcgi_addr_in);

    socket_type = AF_INET;
    fcgi_addr = (struct sockaddr *) &fcgi_addr_in;
  }

  /*
  * Initialize windows sockets library.
  */
  wVersion = MAKEWORD(2,0);
  if (WSAStartup( wVersion, &wsaData )) {
    fprintf(stderr, "%s.%d: error %d starting Windows sockets\n", __FILE__, __LINE__, WSAGetLastError());
  }

  if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
    fprintf(stderr, "%s.%d\n", __FILE__, __LINE__);
    return -1;
  }

  if (-1 == connect(fcgi_fd, fcgi_addr, servlen)) {
    /* server is not up, spawn in  */

    int val;

    if (unixsocket)
      unlink(unixsocket);

    closesocket(fcgi_fd);

    /* reopen socket */
    if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
      fprintf(stderr, "%s.%d\n", __FILE__, __LINE__);
      return -1;
    }

    val = 1;
    if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, 	(const char*)&val, sizeof(val)) < 0) {
      fprintf(stderr, "%s.%d\n", __FILE__, __LINE__);
      return -1;
    }

    /* create socket */
    if (-1 == bind(fcgi_fd, fcgi_addr, servlen)) {
      fprintf(stderr, "%s.%d: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
      return -1;
    }

    if (-1 == listen(fcgi_fd, 1024)) {
      fprintf(stderr, "%s.%d: fd = -1\n", __FILE__, __LINE__);
      return -1;
    }
  } else {
    fprintf(stderr, "%s.%d: socket is already used, can't spawn\n", __FILE__, __LINE__);
    return -1;
  }
  
  
  return fcgi_fd;
}

int FastcgiConnect(const char *addr, unsigned short port, const char *unixsocket
                    , /*uid_t uid, gid_t gid, */int mode, int fork_count) {
  int fcgi_fd = Connection(addr, port, unixsocket);
  if (-1 == fcgi_fd)
    return false;

  // TODO: need close GetStdHandle?
  BOOL f = SetStdHandle(STD_INPUT_HANDLE, (HANDLE)fcgi_fd);
  ASSERT(f);

  f = SetStdHandle(STD_OUTPUT_HANDLE, INVALID_HANDLE_VALUE);
  ASSERT(f);

  f = SetStdHandle(STD_ERROR_HANDLE, INVALID_HANDLE_VALUE);
  ASSERT(f);
  return fcgi_fd;
}

}

#else // POSIX

# include <sys/socket.h>
# include <sys/ioctl.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/un.h>
# include <sys/wait.h>
# include <arpa/inet.h>

# include <netdb.h>

namespace cwf {

int bind_socket(const char *addr, unsigned short port, const char *unixsocket, uid_t uid, gid_t gid, int mode) {
  int fcgi_fd, socket_type, val;

  struct sockaddr_un fcgi_addr_un;
  struct sockaddr_in fcgi_addr_in;
#ifdef USE_IPV6
  struct sockaddr_in6 fcgi_addr_in6;
#endif
  struct sockaddr *fcgi_addr;

  socklen_t servlen;

  if (unixsocket) {
    memset(&fcgi_addr_un, 0, sizeof(fcgi_addr_un));

    fcgi_addr_un.sun_family = AF_UNIX;
    strcpy(fcgi_addr_un.sun_path, unixsocket);

#ifdef SUN_LEN
    servlen = SUN_LEN(&fcgi_addr_un);
#else
    /* stevens says: */
    servlen = strlen(fcgi_addr_un.sun_path) + sizeof(fcgi_addr_un.sun_family);
#endif
    socket_type = AF_UNIX;
    fcgi_addr = (struct sockaddr *) &fcgi_addr_un;

    /* check if some backend is listening on the socket
    * as if we delete the socket-file and rebind there will be no "socket already in use" error
    */
    if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
      fprintf(stderr, "spawn-fcgi: couldn't create socket: %s\n", strerror(errno));
      return -1;
    }

    if (0 == connect(fcgi_fd, fcgi_addr, servlen)) {
      fprintf(stderr, "spawn-fcgi: socket is already in use, can't spawn\n");
      close(fcgi_fd);
      return -1;
    }

    /* cleanup previous socket if it exists */
    if (-1 == unlink(unixsocket)) {
      switch (errno) {
    case ENOENT:
      break;
    default:
      fprintf(stderr, "spawn-fcgi: removing old socket failed: %s\n", strerror(errno));
      return -1;
      }
    }

    close(fcgi_fd);
  } else {
    memset(&fcgi_addr_in, 0, sizeof(fcgi_addr_in));
    fcgi_addr_in.sin_family = AF_INET;
    fcgi_addr_in.sin_port = htons(port);

    servlen = sizeof(fcgi_addr_in);
    socket_type = AF_INET;
    fcgi_addr = (struct sockaddr *) &fcgi_addr_in;

#ifdef USE_IPV6
    memset(&fcgi_addr_in6, 0, sizeof(fcgi_addr_in6));
    fcgi_addr_in6.sin6_family = AF_INET6;
    fcgi_addr_in6.sin6_port = fcgi_addr_in.sin_port;
#endif

    if (addr == NULL) {
      fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef HAVE_INET_PTON
    } else if (1 == inet_pton(AF_INET, addr, &fcgi_addr_in.sin_addr)) {
      /* nothing to do */
#ifdef HAVE_IPV6
    } else if (1 == inet_pton(AF_INET6, addr, &fcgi_addr_in6.sin6_addr)) {
      servlen = sizeof(fcgi_addr_in6);
      socket_type = AF_INET6;
      fcgi_addr = (struct sockaddr *) &fcgi_addr_in6;
#endif
    } else {
      fprintf(stderr, "spawn-fcgi: '%s' is not a valid IP address\n", addr);
      return -1;
#else
    } else {
      if ((in_addr_t)(-1) == (fcgi_addr_in.sin_addr.s_addr = inet_addr(addr))) {
        fprintf(stderr, "spawn-fcgi: '%s' is not a valid IPv4 address\n", addr);
        return -1;
      }
#endif
    }
  }


  if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
    fprintf(stderr, "spawn-fcgi: couldn't create socket: %s\n", strerror(errno));
    return -1;
  }

  val = 1;
  if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
    fprintf(stderr, "spawn-fcgi: couldn't set SO_REUSEADDR: %s\n", strerror(errno));
    return -1;
  }

  if (-1 == bind(fcgi_fd, fcgi_addr, servlen)) {
    fprintf(stderr, "spawn-fcgi: bind failed: %s\n", strerror(errno));
    return -1;
  }

  if (unixsocket) {
    if (0 != uid || 0 != gid) {
      if (0 == uid) uid = -1;
      if (0 == gid) gid = -1;
      if (-1 == chown(unixsocket, uid, gid)) {
        fprintf(stderr, "spawn-fcgi: couldn't chown socket: %s\n", strerror(errno));
        close(fcgi_fd);
        unlink(unixsocket);
        return -1;
      }

      if (-1 == chmod(unixsocket, mode)) {
        fprintf(stderr, "spawn-fcgi: couldn't chmod socket: %s\n", strerror(errno));
        close(fcgi_fd);
        unlink(unixsocket);
        return -1;
      }
    }
  }

  if (-1 == listen(fcgi_fd, 1024)) {
    fprintf(stderr, "spawn-fcgi: listen failed: %s\n", strerror(errno));
    return -1;
  }

  return fcgi_fd;
}

int FastcgiConnect(const char *addr, unsigned short port, const char *unixsocket
                    , /*uid_t uid, gid_t gid, */int mode, int fork_count) {
  int fcgi_fd = bind_socket(addr, port, unixsocket, 0, 0, mode);
  if (-1 == fcgi_fd)
    return 0;

  // TODO: pid_file
  bool nofork = false;
  int child_count = 0;

  int status, rc = 0;
  struct timeval tv = { 0, 100 * 1000 };

  pid_t child;
  int to_fork = fork_count;

  while (to_fork-- > 0) {
    if (!nofork) {
      child = fork();
    } else {
      child = 0;
    }

    switch (child) {
    case 0: {
      char cgi_childs[64];
      int max_fd = 0;

      int i = 0;

      if (child_count >= 0) {
        snprintf(cgi_childs, sizeof(cgi_childs), "PHP_FCGI_CHILDREN=%d", child_count);
        putenv(cgi_childs);
      }

      if(fcgi_fd != FCGI_LISTENSOCK_FILENO) {
        close(FCGI_LISTENSOCK_FILENO);
        dup2(fcgi_fd, FCGI_LISTENSOCK_FILENO);
        close(fcgi_fd);
      }

      /* loose control terminal */
      if (!nofork) {
        setsid();

        max_fd = open("/dev/null", O_RDWR);
        if (-1 != max_fd) {
          if (max_fd != STDOUT_FILENO) dup2(max_fd, STDOUT_FILENO);
          if (max_fd != STDERR_FILENO) dup2(max_fd, STDERR_FILENO);
          if (max_fd != STDOUT_FILENO && max_fd != STDERR_FILENO) close(max_fd);
        } else {
          fprintf(stderr, "spawn-fcgi: couldn't open and redirect stdout/stderr to '/dev/null': %s\n", strerror(errno));
        }
      }

      /* we don't need the client socket */
      for (i = 3; i < max_fd; i++) {
        if (i != FCGI_LISTENSOCK_FILENO) close(i);
      }

#if 0
      /* fork and replace shell */
      if (appArgv) {
        execv(appArgv[0], appArgv);

      } else {
        char *b = malloc((sizeof("exec ") - 1) + strlen(appPath) + 1);
        strcpy(b, "exec ");
        strcat(b, appPath);

        /* exec the cgi */
        execl("/bin/sh", "sh", "-c", b, (char *)NULL);
      }
#endif

      /* in nofork mode stderr is still open */
      fprintf(stderr, "spawn-fcgi: exec failed: %s\n", strerror(errno));
      // exit(errno);
      return -1;

      break;
            }
    case -1:
      /* error */
      fprintf(stderr, "spawn-fcgi: fork failed: %s\n", strerror(errno));
      break;
    default:
      /* father */

      /* wait */
      select(0, NULL, NULL, NULL, &tv);

      switch (waitpid(child, &status, WNOHANG)) {
    case 0:
      fprintf(stdout, "spawn-fcgi: child spawned successfully: PID: %d\n", child);
#if 0
      /* write pid file */
      if (pid_fd != -1) {
        /* assume a 32bit pid_t */
        char pidbuf[12];

        snprintf(pidbuf, sizeof(pidbuf) - 1, "%d", child);

        write(pid_fd, pidbuf, strlen(pidbuf));
        /* avoid eol for the last one */
        if (fork_count != 0) {
          write(pid_fd, "\n", 1);
        }
      }
#endif

      break;
    case -1:
      break;
    default:
      if (WIFEXITED(status)) {
        fprintf(stderr, "spawn-fcgi: child exited with: %d\n",
          WEXITSTATUS(status));
        rc = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "spawn-fcgi: child signaled: %d\n",
          WTERMSIG(status));
        rc = 1;
      } else {
        fprintf(stderr, "spawn-fcgi: child died somehow: exit status = %d\n",
          status);
        rc = status;
      }
      }

      break;
    }
  }
  // close(pid_fd);

  if (fork_count)
    close(fcgi_fd);

  return fcgi_fd;
}

}
#endif
