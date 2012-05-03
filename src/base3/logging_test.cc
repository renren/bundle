#include <gtest/gtest.h>
#include <boost/thread.hpp>
#include "base3/logging.h"
#include <iostream>
#include <fstream>
#include <errno.h> 



// 是否被调用过的测试类
struct foo {
  foo(const char* name="") : name_(name) {
    std::cout << name_ << " foo::foo\n";
  }
  ~foo() {
    std::cout << name_ << " foo::~foo\n";
  }  
  std::string name_;
};

std::ostream& operator<<(std::ostream& out, const foo&) {
  // nothing here
  return out;
}

TEST(LoggingTest, Macro) {
  LOG(INFO) << "info text number: " << 42;

  LOG_IF(INFO, 3>4) << "never appear in log";
  LOG_IF(INFO, 4>3) << "should appear in log";

  LOG_ASSERT(1==2);

  DLOG(INFO) << "debug log = old verbose";
  DLOG_IF(INFO, 2 > 3);
  DLOG_ASSERT(1==2);

  // TODO: check log content

  // TODO: 
  VLOG(1) << "verbose log";
  VLOG_IS_ON(1);

  PLOG(INFO) << "with error number";
}

TEST(LoggingTest, Reopen) {
  logging::ReopenLogFile();

  LOG(INFO) << "after reopened";
}