#include "gtest/gtest.h"

#include <dlfcn.h>

TEST(Dynaload, Test) {
  {
    const char *p = "dynaload write before dlopen\n";
    write(2, p, strlen(p));
  }

  void *handle = dlopen("libpreloadfs.so", RTLD_NOW);
  ASSERT_TRUE(handle) << dlerror();

  {
    char *p = "dynaload write after dlopen\n";
    write(2, p, strlen(p));
  }

  dlclose(handle);

  {
    const char *p = "dynaload write after dlclose\n";
    write(2, p, strlen(p));
  }
}



// LD_LIBRARY_PATH=out/Default/lib.target/ out/Default/dynaload_test