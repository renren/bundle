#include <cstdio>
#include <vector>
#include <iostream>
#include <fstream>

// TODO: include fastcgi.h
#define FCGI_LISTENSOCK_FILENO 0

#if defined(OS_LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <netinet/in.h>
# include <sys/un.h>
# include <sys/wait.h>
#endif

#include "base3/getopt_.h"
#include "base3/logging.h"
#include "base3/signals.h"
#include "base3/startuplist.h"

#include "cwf/frame.h"
#include "cwf/connect.h"

#if defined(POSIX) || defined(OS_LINUX)
#include <unistd.h>
#include <errno.h>
#endif

static void show_help () {
  puts("Usage: cwf [options] [-- <fcgiapp> [fcgi app arguments]]\n" \
    "\n" \
    "Options:\n" \
    " -e             dump effective Action(s)\n" \
    " -d <directory> chdir to directory before spawning\n" \
    " -a <address>   bind to IPv4/IPv6 address (defaults to 0.0.0.0)\n" \
    " -p <port>      bind to TCP-port\n" \
    " -l <file>      log file name\n" \
    " -t <thread>    thread count\n" \
    " -s <path>      bind to Unix domain socket\n" \
    " -M <mode>      change Unix domain socket mode\n" \
    " -C <children>  (PHP only) numbers of childs to spawn (default: not setting\n" \
    "                the PHP_FCGI_CHILDREN environment variable - PHP defaults to 0)\n" \
    " -F <children>  number of children to fork (default 1)\n" \
    " -P <path>      name of PID-file for spawned process (ignored in no-fork mode)\n" \
    " -n             no fork (for daemontools)\n" \
    " -v             show version\n" \
    " -?, -h         show this help\n" \
    "(root only)\n" \
    " -c <directory> chroot to directory\n" \
    " -S             create socket before chroot() (default is to create the socket\n" \
    "                in the chroot)\n" \
  );
}

void OpenLogger(const char * log_filename) {
  if (!log_filename) // ugly check
    return;

  using namespace logging;
  InitLogging(log_filename, LOG_ONLY_TO_FILE
    , DONT_LOCK_LOG_FILE
    , APPEND_TO_OLD_LOG_FILE);

  // pid, thread_id, timestamp, tickcount
  SetLogItems(true, true, true, false);
}

#if defined(OS_LINUX)
int Daemon() {
  switch (fork()) {
    case -1:
      PLOG(ERROR) << "fork() failed";
      return -1;

    case 0:
      break;

    default:
      exit(0);
  }

  pid_t pid = getpid();

  if (setsid() == -1) {
    PLOG(ERROR) << "setsid() failed";
    return -1;
  }

  umask(0);

  int fd = open("/dev/null", O_RDWR);
  if (fd == -1) {
    PLOG(ERROR) << "open(\"/dev/null\") failed";
    return -1;
  }

  if (dup2(fd, STDIN_FILENO) == -1) {
    PLOG(ERROR) << "dup2(STDIN) failed";
    return -1;
  }

  if (dup2(fd, STDOUT_FILENO) == -1) {
    PLOG(ERROR) << "dup2(STDOUT) failed";
    return -1;
  }

  if (dup2(fd, STDERR_FILENO) == -1) {
    LOG(ERROR) << "dup2(STDERR) failed";
    return -1;
  }

  return 0;
}

volatile int fork_count_ = 0;
volatile int quit_ = 0;
std::vector<int> * children_ = 0;

void KillChildren(int sig) {
  if (!children_) 
    return;

  for (std::vector<int>::const_iterator i=children_->begin();
      i!=children_->end(); ++i) {
    int pid = *i;
    int ret = kill(pid, sig);
    LOG(INFO) << "kill " << pid << " " << sig << " ret:" << ret;
  }
}

void SignalTerminate(int) {
  quit_ = 1;

  KillChildren(SIGINT);
}

void SignalReopen(int) {
  logging::ReopenLogFile();

  KillChildren(SIGUSR1);
}

void SignalUpdate(int) {
  // 构造 env CWFLISTEN=%ud
  // execv()
}

int GetInheritedSocket() {
  return 0;
}

void SignalChildren(int) {
  fork_count_ = 1;

  int status = 0;
  for (;;) {
    int pid = waitpid(-1, &status, WNOHANG);
    if (0 == pid)
      return;

    if (-1 == pid) {
      PLOG(INFO) << "master waitpid"; // TODO: errno = EINTR, ECHLD
      return;
    }

    if (WIFEXITED(status))
      LOG(INFO) << "master waitpid: " << WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
      LOG(INFO) << "master waitpid: " << WTERMSIG(status);
    else
      LOG(INFO) << "master waitpid: exit status = " << status;
  }
}

// return > 1 for child pid
// return 0 for child
// return < 0 for error
int Fork(int fcgi_fd, int thread_count, const char * log_filename) {
  int rc = 0;
  pid_t child = fork();

  if (-1 == child) {
    PLOG(ERROR) << "master fork failed";
    return -1;
  }

  // parent
  if (child) {
    struct timeval tv = { 0, 100 * 1000 };
    select(0, NULL, NULL, NULL, &tv);

    int status = 0;
    switch (waitpid(child, &status, WNOHANG)) {
      case 0:
        LOG(INFO) << "child spawned successfully, PID: " << child;
        break;

      default:
        if (WIFEXITED(status))
          LOG(INFO) << "child exited with: " << WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
          LOG(INFO) << "child signaled: " << WTERMSIG(status);
        else
          LOG(INFO) << "child died somehow: exit status = " << status;
    }
    return child;
  }

  // child
  if (0 == child) {
    // ChildrenCycle for reopen log, quit
    LOG(INFO) << "child proccess begin";

    if(fcgi_fd != FCGI_LISTENSOCK_FILENO) {
      // LOG(INFO) << "change FCGI_LISTENSOCK_FILENO";
      close(FCGI_LISTENSOCK_FILENO);
      dup2(fcgi_fd, FCGI_LISTENSOCK_FILENO);
      close(fcgi_fd);

      fcgi_fd = FCGI_LISTENSOCK_FILENO;
    }

    base::InstallSignal(SIGUSR1, SignalReopen);

    // 子进程退出在frame.cc里实现
    cwf::FastcgiMain(thread_count, fcgi_fd);
    return 0;
  }

  return rc;
}

int MasterCycle(int thread_count, int fcgi_fd, const char * log_filename) {
  base::InstallSignal(SIGCHLD, SignalChildren);

  base::InstallSignal(SIGINT, SignalTerminate);
  base::InstallSignal(SIGTERM, SignalTerminate);

  base::InstallSignal(SIGUSR1, SignalReopen);
  base::InstallSignal(SIGUSR2, SignalUpdate);

  sigset_t           set;
  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  sigaddset(&set, SIGALRM);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGTERM);

  while (true) {
    int ret = pause();
    LOG(INFO) << "supspend once" << ret;

    if (fork_count_) {
      int child = Fork(fcgi_fd, thread_count, log_filename);
      if (0 == child)
        return 0;

      children_->push_back(child);

      fork_count_ = 0;
    }

    if (quit_) {
      // quit children

      LOG(INFO) << "normal quit";
      break;
    }
  }

  return 0;
}
#endif

int main(int argc, char* argv[]) {
  int port = 3000;
  const char * addr = "0.0.0.0";
  const char * unixsocket = 0;
  int sockmode = 0; // 缺省值从 -1 改为 0 了
  char * fcgi_dir = 0;
  char * log_filename = 0;
  char * pid_file = 0;
  int thread_count = 0;
  int nofork = 0;
  int fork_count = 0;

  int o;

  while (-1 != (o = getopt(argc, argv, "a:d:F:M:p:t:l:s:P:?hen"))) {
    switch(o) {
    case 'e' : 
      // disable std::err log
      logging::SetMinLogLevel(logging::LOG_FATAL);
      base::RunStartupList();
      cwf::FrameWork::ListAction(std::cout);
      base::DestoryStartupList();
      return 0;
    case 'd': fcgi_dir = optarg; break;
    case 'l': log_filename = optarg; break;
    case 'a': addr = optarg;/* ip addr */ break;
    case 'p': 
      {
        char *endptr = 0;
        port = strtol(optarg, &endptr, 10);/* port */
        if (*endptr) {
          fprintf(stderr, "spawn-fcgi: invalid port: %u\n", (unsigned int) port);
          return -1;
        }
      }
      break;
    case 't': thread_count = strtol(optarg, NULL, 10);/*  */ break;
    case 'F': fork_count = strtol(optarg, NULL, 10);/*  */ break;
    case 's': unixsocket = optarg; /* unix-domain socket */ break;
//    case 'c': if (i_am_root) { changeroot = optarg; }/* chroot() */ break;
//    case 'u': if (i_am_root) { username = optarg; } /* set user */ break;
//    case 'g': if (i_am_root) { groupname = optarg; } /* set group */ break;
//    case 'U': if (i_am_root) { sockusername = optarg; } /* set socket user */ break;
//    case 'G': if (i_am_root) { sockgroupname = optarg; } /* set socket group */ break;
//    case 'S': if (i_am_root) { sockbeforechroot = 1; } /* open socket before chroot() */ break;
    case 'M': sockmode = strtol(optarg, NULL, 0); /* set socket mode */ break;
    case 'n': nofork = 1; break;
    case 'P': pid_file = optarg; /* PID file */ break;
//    case 'v': show_version(); return 0;
    case '?':
    case 'h': show_help(); return 0;
    default:
      show_help();
      return 0;
    }
  }

  OpenLogger(log_filename);

#if defined(POSIX) || defined(OS_LINUX)
  if (fcgi_dir && -1 == chdir(fcgi_dir)) {
    fprintf(stderr, "spawn-fcgi: chdir('%s') failed: %s\n", fcgi_dir, strerror(errno));
    return -1;
  }
#endif

  // socket
#if defined(OS_LINUX)
  int fcgi_fd = cwf::bind_socket(addr, port, unixsocket, 0, 0, sockmode);
#elif defined(OS_WIN)
  int fcgi_fd = cwf::Connection(addr, port, unixsocket);
#endif
  if (-1 == fcgi_fd) {
    PLOG(ERROR) << "socket error";
    return -1;
  }

#if defined(OS_LINUX)
  if (!nofork && 0 != Daemon())
    return -1;

  if (pid_file) {
    // TODO: open with CLOSE_REMOVE
    std::ofstream pidfile(pid_file);
    if (pidfile)
      pidfile << getpid() << "\n";
  }

  if (fork_count) {
    std::vector<int> children;

    int c = fork_count;
    while (c--) {
      int child = Fork(fcgi_fd, thread_count, log_filename);
      if (0 == child)
        return 0;

      children.push_back(child);
    }

    children_ = new std::vector<int>();
    children_->swap(children);

    return MasterCycle(thread_count, fcgi_fd, log_filename);
  }
#endif

  return cwf::FastcgiMain(thread_count, fcgi_fd, log_filename);

#if 0
  int rc = cwf::FastcgiConnect(addr, port, unixsocket, sockmode, fork_count);
#if defined(POSIX) || defined(OS_LINUX)
  // rc = 0 表示parent 执行成功
  // rc = -1 表示 child 成功
  // 其他，失败
  if (rc > 0 && fork_count > 0) {
    return 0;
  }

  // child
  if (rc == -1 && fork_count > 0) {
    return cwf::FastcgiMain(thread_count, 0, log_filename);
  }
#endif
#endif
}
