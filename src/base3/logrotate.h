#ifndef BASE_BASE_LOG_ROTATE_H__
#define BASE_BASE_LOG_ROTATE_H__

#include <string>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#ifdef OS_WIN
#include <direct.h>
#endif
#include <boost/date_time/posix_time/posix_time.hpp>

#include "base3/common.h"
#include "base3/basictypes.h"
#include "base3/pathops.h"
#include "base3/mkdirs.h"
#include "base3/directstream.h"
#include "base3/baseasync.h"
#include "base3/logging.h"
#include "base3/signals.h"

namespace base {

struct LogRotate {
  LogRotate() : level_(-1) {}

  //  input            =>   normalize
  // logs/talkright01  => logs/talkright01/
  // ./cwf.pid/        => /cwd/cwf.pid/

  bool Start(const std::string& path, int level) {
    // Open
    // signal handler
    // start timer, 每日24:00轮转

    filename_ = path;
    level_ = level;

    ASSERT(IsDirectory(path));

    NormalizeDirectory(&filename_);
    if (!IsAbspath(filename_)) {
#ifdef OS_WIN
#define getcwd _getcwd
#endif
      char* cwdbuf = new char[260];   // new to avoid stack overflow
      const char* cwd = getcwd(cwdbuf, 260);

      filename_ = PathJoin(cwd, filename_);
      delete[] cwdbuf;
    }

      // 创建目录
    struct stat st;
    if (0 != stat(filename_.c_str(), &st)) {
#ifdef OS_LINUX
      mkdirs(filename_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
      mkdirs(filename_.c_str(), 0);
#endif
    }

    filename_ += "oce_log";

    std::cout << "log to " << filename_ << " level:" << level << "\n";

    Open();

#ifdef OS_LINUX
    struct sigaction sa;
    sa.sa_sigaction = LogRotate::SignalHandler;
    // TODO: 是否需要 RESTART
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    int ret = sigaction(BASE_SIGNAL_LOG_OPEN, &sa, NULL);
    ASSERT(ret == 0);
    ret = sigaction(BASE_SIGNAL_LOG_ROTATE, &sa, NULL);
    ASSERT(ret == 0);
#endif

    // 每日24:00轮转
    timer_.reset(new boost::asio::deadline_timer(
      detail::Async::GetService(), NextTime()
      ));
    timer_->async_wait(boost::bind(&LogRotate::AutoRotate, this));
    return true;
  }

  void Stop() {
    Close();
    timer_.reset(0);
  }

  void Open() {
    std::ostream* stem = 0;
    if (!filename_.empty()) {
#ifdef OS_LINUX
      stem = new DirectOutStream(filename().c_str(), std::ios::app);
#else
      stem = new std::ofstream(filename().c_str(), std::ios::app);
#endif
    }

    logging::SetMinLogLevel(level_);
  }

  void Close() {
    logging::CloseLogFile();
  }

  void Rotate() {
    // 实现轮转日志，有2种实现方案
    // 1 自己写代码, copy from logrotate-3.7.8
    //   枚举所有日志文件，改名
    //   然后close stream
    //   open new

    // 2 也可以生成一个临时配置文件，让 logrotate 执行
    // 如果当前目录有 rotate.conf，就不管
    //   没有，自己在/tmp里生成一个 conf，调用logrotate
    // logrotate -s /pwd/status -f /tmp/a.conf
#if 0
    /path/to/oce_log {
      rotate 3
        daily
        postrotate
        kill -BASE_SIGNAL_LOG_OPEN getpid()
        endscript
    }
#endif

    // 1 文件是否存在
    {
      std::string conf(filename_ + "/rotate.conf");

#ifdef OS_LINUX
      struct stat st;
      int res = ::stat(conf.c_str(), &st);
#elif defined(OS_WIN)
      struct __stat64 st;
      int res = ::_stat64(conf.c_str(), &st);

#define tempnam _tempnam // ugly
#endif
      if (res == 0) // file exist
        return;
    }

    // 2 生成临时文件 /tmp/xx.conf
    std::string conf;
    {
      char* p = tempnam("/tmp", "arch");
      if (!p)
        return;

      conf = p;
      std::ofstream conf_stem(conf.c_str()); // , std::ios::trunc);
      if (!conf_stem)
        return;

      conf_stem << filename() << " {\n"
        << "  rotate 3\n"
        "  daily\n"
        "  postrotate\n"
        "    kill -" << BASE_SIGNAL_LOG_OPEN << " " << getpid() << "\n"
        << "  endscript\n"
        "}\n";
    }

    // 3 call system logrotate -s /pwd/status -f /tmp/a.conf
    std::string cmd("/usr/sbin/logrotate -s ");
    cmd += Dirname(filename_);
    cmd += "/rotate.staus -f ";
    cmd += "\"";
    cmd += conf;
    cmd += "\"";

    int ret = system(cmd.c_str());
    ASSERT(0 == ret);
    remove(conf.c_str());
  }

  void AutoRotate() {
    Rotate();

    timer_->expires_at(NextTime());
    timer_->async_wait(boost::bind(&LogRotate::AutoRotate, this));
  }

#ifdef OS_LINUX
  static void SignalHandler(int sig, siginfo_t* sinfo, void* ucontext) {
    // LOG(INFO) << "got reopen signal " << sig;
    LogRotate& r = instance();
    if (r.filename_.empty() || -1 == r.level_)
      return;
    
    if (sig == BASE_SIGNAL_LOG_OPEN)
      r.Open();
    else if(BASE_SIGNAL_LOG_ROTATE == sig)
      r.Rotate();
  }
#endif

  static int LogLevel(std::string const& desc) {
    const char* arr[] = {
      "DEBUG", // VERBOSE
      "INFO",
      "WARNING",
      "ERROR"
    };
    for (int i=0; i<ARRAY_SIZE(arr); ++i)
      if (desc == arr[i])
        return i;
    return -1;
  }

  static LogRotate& instance() {
    static LogRotate r_;
    return r_;
  }

  static boost::posix_time::ptime NextTime() {
    using namespace boost::gregorian;
    // using namespace boost::posix_time;

    boost::posix_time::ptime tomorrow;
    // ugly, TODO: better impl
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    date today = now.date(); // 去除时间
    tomorrow = boost::posix_time::ptime(today + days(1)); // 12:00
    // std::cout << to_simple_string(tomorrow) << std::endl;
    return tomorrow;
  }

  const std::string & filename() const {
    return filename_;
  }

private:
  DISALLOW_EVIL_CONSTRUCTORS(LogRotate);

  std::string filename_;
  int level_;
  std::auto_ptr<boost::asio::deadline_timer> timer_;
};

}
#endif // BASE_BASE_LOG_ROTATE_H__
