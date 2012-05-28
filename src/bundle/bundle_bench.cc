#include "gtest/gtest.h"

#include <stdlib.h>

#include "base3/ptime.h"
#include "bundle/bundle.h"

#if defined(USE_MOOSECLIENT)
#include "mooseclient/moose_c.h"
#endif

TEST(Bundle, Allocate) {
  using namespace bundle;

  {
    base::ptime pt("run 10000");
    int c = 10000;
    while(c--) {
      const int file_size = random() % (5 * 1024 * 1024);

      Writer *writer = Writer::Allocate("p/20120512", ".jpg", file_size, "test");

      std::string url = writer->EnsureUrl();

      writer->Release();
    }
  }
}


#if defined(USE_MOOSECLIENT)

int main(int argc, char **argv) {
  std::cout << "Running main() from bundle_benc.cc\n";

  bool connected = false;

  for (int i=1; i<argc; ++i) {
    if (std::string(argv[i]) == "-H") {
      int ret = mfs_connect(argv[i+1]);
      std::cout << "Connect to " << argv[i+1] << " return " << ret << std::endl;
      connected = true;
      break;
    }
  }

  if (!connected) {
    std::cerr << " Error: run as -H 127.0.0.1:9421 connect to moose master.\n";
    return 1;
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif
