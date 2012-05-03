#include <gtest/gtest.h>
#include <iostream>
#include "base3/common.h"

using namespace base;

TEST(TCMalloc, MemoryUseage) {
  int b = CurrentMemoryUsage();

  char *p = new char[10000];

  int e = CurrentMemoryUsage();
  EXPECT_GT(e, b);

  delete [] p;

  int e2 = CurrentMemoryUsage();
  EXPECT_TRUE(e2 <= e);
  EXPECT_TRUE(e2 >= b);

  std::cout << b << ", " << e << ", " << e2 << "\n";
}

TEST(TCMalloc, Leak) {
  char* p = new char[42];
  p[0] = 'a';
}
