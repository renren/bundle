#include <iostream>
#include <gtest/gtest.h>
#include <boost/detail/atomic_count.hpp>

#include "base3/baseasync.h"
#include "base3/common.h"

static boost::detail::atomic_count bark_count_(0);
static void bark() {
  ++ bark_count_;
  std::cout << "bark once\n";
}

static int foo() {
  bark();
  return 42;
}

int foo2(int a) {
  bark();
  return a;
}


TEST(BaseAsyncTest,compile) {
  using namespace base::detail;
  Async::Start();
  
  // Async::Post(&foo);
  // Async::Post(&foo, 3);

  Async::Post(&foo, 800, 100);
  Async::Post(&foo, 1200, 100);
  base::Sleep(1);

  Async::Stop();
}
