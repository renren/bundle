#include <gtest/gtest.h>

#include "base3/common.h"
#include <iostream>

TEST(CommonTest, Memoryusage) {
  size_t a = base::CurrentMemoryUsage();
  char* f = new char[1024];
  size_t b = base::CurrentMemoryUsage();

  // std::cout << a << " " << b << std::endl;
  EXPECT_LT(a, b);
}
