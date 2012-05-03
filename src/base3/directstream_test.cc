#include <gtest/gtest.h>
#include "base3/directstream.h"

TEST(DirectStreamTest, All) {
#ifdef OS_LINUX
  std::ostream* ostem = new base::DirectOutStream("test");
  char sz[] = "abc";
  ostem->write(sz, 3);
  delete ostem;
#endif
}
