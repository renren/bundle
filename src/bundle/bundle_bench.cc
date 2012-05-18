#include "gtest/gtest.h"

#include <stdlib.h>

#include "bundle/bundle.h"

TEST(Bundle, Allocate) {
  using namespace bundle;

  const int file_size = random() % (5 * 1024 * 1024);

  Writer *writer = Writer::Allocate("p/20120512", ".jpg", file_size, ".");

  std::string url = writer->EnsureUrl();

  writer->Release();
}
