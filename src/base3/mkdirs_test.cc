#include <errno.h>
#include <gtest/gtest.h>

#include "base3/mkdirs.h"

TEST(MkdirsTest, Noname) {
	using namespace base;
#if defined(OS_LINUX)
  int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
  const char* arr[] = {
    "e/f/g/h",
    "e/f/g/g",
    "./a/b",
    "./c/d/",
    "./d",
    //0
  };
#else
  int mode = 0;
  const char* arr[] = {
    "e\\f\\g\\h",
    ".\\a\\b",
    ".\\c\\d\\",
    ".\\d",
    0
  };
#endif
  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    EXPECT_EQ(0, mkdirs(arr[i], mode));
  }

#if 0
  for(const char* p = arr[0]; *p; ++p)
    //mkdirs(p, mode);
    printf("--%s\n",p);
#endif

}

TEST(MkdiesTest, Test) {
  const char* arr[] = {
    ".lock",
    "e/f/g/h",
    "e/f/g/g",
    "e/f/g/g",
    "./a/b",
    "./a/b/c",
    "./c/d/",
    "./d",
    "~/a/b",
    //0
  };
  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    EXPECT_EQ(0, base::mkdirs(arr[i]));
  }


}
