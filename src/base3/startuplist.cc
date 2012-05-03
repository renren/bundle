#include "base3/startuplist.h"

#include <list>
#include <boost/foreach.hpp>

#include "base3/common.h"
#include "base3/logging.h"
#include "base3/once.h"

namespace base {

static std::list<StartupFunction>* startup_list_ = 0;
static bool called_ = false;

static void InitStartupList() {
  startup_list_ = new std::list<StartupFunction>();
}

void AddStartup(StartupFunction func, bool first) {
  static BASE_DECLARE_ONCE(once_guard_);
  BaseOnceInit(&once_guard_, &InitStartupList);

  // 未加锁的，应该不需要
  ASSERT(func);
  if (first)
    startup_list_->push_front(func);
  else
    startup_list_->push_back(func);
}

void RunStartupList() {
  if (!startup_list_)
    return;

  // 一般执行多次应该是无害的
  ASSERT(!called_);
  called_ = true;

  LOG(INFO) << "RunStartupList count: " << startup_list_->size();
  int c = 0;
  BOOST_FOREACH(StartupFunction f, *startup_list_) {
    ++ c;
    f();
  }
  LOG(INFO) << "RunStartupList done";
}

void DestoryStartupList() {
  delete startup_list_;
  startup_list_ = 0;
}

}
