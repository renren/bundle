#ifndef OCE_BASE_PCOUNT_H__
#define OCE_BASE_PCOUNT_H__

#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

#include <boost/detail/atomic_count.hpp>
#include <boost/function.hpp>

#include "base3/common.h"

/*
类似 sar 日志保存位置 /var/log/sa/sa10
[root@TJSJHL ~]$ sar -n SOCK
Linux 2.6.18-128.el5 (TJSJHL219-216.opi.com) 	08/05/2009

12:00:01 AM    totsck    tcpsck    udpsck    rawsck   ip-frag
12:10:01 AM      7095      7920         9         0         0
12:20:01 AM      6251      6891         9         0         0
12:30:01 AM      5042      5774         9         0         0
12:40:01 AM      4784      5193         9         0         0
12:50:01 AM      4416      4843         9         0         0
01:00:01 AM      4068      4538         9         0         0
01:10:01 AM      3510      3735         9         0         0
01:20:01 AM      3182      3412         9         0         0
01:30:01 AM      2897      3082         9         0         0
01:40:01 AM      2597      2773         9         0         0

使用方式:
有个关键服务TaskManager,对有多少未执行的执行task比较敏感
定时输出到日志文件，以便后期分析

1 直接使用
class TaskManager {
public:
  TaskManager() : task_counter_(pcount.push_add("quetask")) {}
  void ExecuteOneTask() {
    pcount.push_inc(task_counter_);
  }

private:
  size_t task_counter_;
};

2 使用宏
class TaskManager {
public:
  TaskManager() : XAR_REGISTER2(task_counter_, "quetask", 0) {
    XAR_REGISTER(task_counter_, "quetask", 0);
  }
  void ExecuteOneTask() {
    XAR_INC(task_counter_);
  }

private:
  XAR_COUNT(task_counter_);
};

*/

namespace base {

class pcount {
public:
  pcount()
    : push_index_(-1) // -1 很诡异的初始值, 目的是保证 push_add 线程安全
    , poll_index_(-1)
  {}

  static pcount& instance() {
    static pcount p_;
    return p_;
  }

  const static int align = 10;

  // ---------------------- push ------------------------
  // 注册一个counter, 返回索引
  size_t push_add(const char* name, int init_count = 0) {
#if BOOST_VERSION <= 103500
    ++push_index_;
    size_t index = push_index_;
#else
    size_t index = ++push_index_; // 线程安全
#endif

    ASSERT(index < SIZE_);
    if (index >= SIZE_)
      index = 0;

    push_name_[index] = name ? name : "";
    push_count_[index] = new atomic_t(init_count);
    return index;
  }

  long push_inc(size_t index) {
#if BOOST_VERSION <= 103500
    ++*push_count_[index];
    return *push_count_[index];
#else
    return ++*push_count_[index];
#endif
  }
  long push_dec(size_t index) {
    return --*push_count_[index];
  }
  size_t push_size() const {
    return 1 + push_index_;
  }

  std::string push_title() const {
    std::ostringstream ostem;
    size_t c = push_size();
    for (size_t i=0; i<c; ++i)
      ostem << std::setfill(' ') << std::setw(align) << push_name_[i];
    return ostem.str();
  }
  std::string push_stat() const {
    std::ostringstream ostem;
    size_t c = push_size();
    for (size_t i=0; i<c; ++i)
      ostem << std::setfill(' ') << std::setw(align) << *push_count_[i];
    return ostem.str();
  }

  // ---------------------- poll ------------------------
  size_t poll_size() const {
    return 1 + poll_index_;
  }

  typedef boost::function0<size_t> poll_type;

  void poll_add(const char* name, const poll_type& func) {
#if BOOST_VERSION <= 103500
    ++poll_index_;
    size_t index = poll_index_;
#else
    size_t index = ++poll_index_; // 线程安全
#endif

    poll_name_[index] = name ? name : "";
    poll_count_[index] = func;
  }

  std::string poll_title() const {
    std::ostringstream ostem;
    size_t c = poll_size();
    for (size_t i=0; i<c; ++i)
      ostem << std::setfill(' ') << std::setw(align) << poll_name_[i];
    return ostem.str();
  }
  std::string poll_stat() const {
    std::ostringstream ostem;
    size_t c = poll_size();
    for (size_t i=0; i<c; ++i)
      ostem << std::setfill(' ') << std::setw(align) << poll_count_[i]();
    return ostem.str();
  }

private:
  const static size_t SIZE_ = 60; // 缺省尺寸

  typedef boost::detail::atomic_count atomic_t;
  atomic_t* push_count_[SIZE_];  
  std::string push_name_[SIZE_];
  boost::detail::atomic_count push_index_; // 1 + push_index_ 才是下一个的索引

  poll_type poll_count_[SIZE_];
  std::string poll_name_[SIZE_];
  boost::detail::atomic_count poll_index_;
};

class xar {
public:
  xar() : push_size_(0)
    , interval_(5), line_(0) {}

  static xar& instance() {
    static xar p_;
    return p_;
  }

  const std::string& filename() const {
    return filename_;
  }
  void set_filename(const std::string& filename) {
    filename_ = filename;
  }

  // 秒数
  size_t interval_seconds() const {
    return interval_;
  }
  void set_interval_seconds(size_t interval) {
    interval_ = interval;
  }

  // 00:00:00
  // move to xar::output_stat?
  static std::string current_time() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[32];

    time(&rawtime);
    timeinfo = localtime( &rawtime );

    strftime(buffer, sizeof(buffer),"%H:%M:%S", timeinfo);
    return buffer;
  }

  void output(std::ostream& ostem) {
    pcount& x = pcount::instance();

    // 如果 title变更了，需要重新输出title
    if (push_size_ != x.push_size() || line_ % 40 == 0) {
      ostem << std::string(8, ' '); // time      
      ostem << x.poll_title()
        << x.push_title()
        << rusage_title() << "\n";

      push_size_ = x.push_size();
    }

    ostem << current_time() << x.poll_stat()
      << x.push_stat()
      << rusage() << std::endl;
    ++ line_;
  }
  
  static size_t rusage_size;
  static const char* rusage_name[];

  static std::string rusage_title() {
    std::ostringstream ostem;
    for (size_t i=0; i<rusage_size; ++i)
      ostem << std::setfill(' ') << std::setw(pcount::align) << rusage_name[i];
    return ostem.str();
  }

  static std::string rusage();

  // 必须显示调用，避免编译so时可能不链接
  static bool start();

private:
  size_t push_size_;
  std::string filename_;
  size_t interval_;
  size_t line_;
};

}

#ifndef BASE_NO_XAR

// 注册一个整数，有变化是就调用
#define XAR_DECLARE(name) \
  size_t PIDX_##name

#define XAR_IMPL(name) \
  size_t PIDX_##name = base::pcount::instance().push_add(#name, 0)

// 一般用在构造列表
#define XAR_INIT(name) \
  PIDX_##name(base::pcount::instance().push_add(#name, 0))

#define XAR_INC(name) \
  base::pcount::instance().push_inc(PIDX_##name)

#define XAR_DEC(name) \
  base::pcount::instance().push_dec(PIDX_##name)

// 注册function, xar::output 时调用
#define XAR_POLL(name, func) \
  base::pcount::instance().poll_add(name, func)

#else // BASE_NO_XAR

#define XAR_DECLARE(name)
#define XAR_IMPL(name)
#define XAR_INIT(name)
#define XAR_INC(index)
#define XAR_DEC(index)

#define XAR_POLL(index)

#endif

#endif
