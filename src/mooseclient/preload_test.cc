#include "gtest/gtest.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mooseclient/preload.h"

extern std::string ToInterpath(std::string const &abspath);
extern bool IsTrappedFile(const std::string &pathname);
extern std::string ToAbspath(const char *pathname);

TEST(Preload, Pathutil) {
  {
    struct {
      const char *p;
      bool trapped;
    } arr [] = {
      {"/mnt", false},
      {"/mnt/mfs/", true},
      {"/mnt/mfs", true},
      {"/mnt/mfs/1", true},
      {"/mnt/mfs/1/", true},
      {"/mnt/mfs3/", false},
      {"/mnt/mf", false},
      {"/mnt/mf/", false},
      {"/mnt/mfd/", false},
    };

    SetMountPoint("/mnt/mfs");

    for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
      EXPECT_EQ(arr[i].trapped, IsTrappedFile(arr[i].p)) << arr[i].p;

    SetMountPoint("/mnt/mfs/");

    for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
      EXPECT_EQ(arr[i].trapped, IsTrappedFile(arr[i].p)) << arr[i].p;
  }

  // -------------------------------------------------
  {
    int ret = chdir("/tmp");
    ASSERT_EQ(0, ret);

    struct {
      const char *name;
      const char *abspath;
    } brr[] = {
      {"foo", "/tmp/foo"},
      {"foo/", "/tmp/foo/"},
      {"foo/bar", "/tmp/foo/bar"},
      {"foo/bar/", "/tmp/foo/bar/"},
      {"/foo", "/foo"},
      {"/", "/"},
      {"", "/tmp"},
    };

    for (int i=0; i<sizeof(brr)/sizeof(brr[0]); i++)
      EXPECT_EQ(std::string(brr[i].abspath), ToAbspath(brr[i].name)); // << brr[i].name;
  }

  // -------------------------------------------------
  {
    struct {
      const char *a;
      const char *b;
    } brr[] = {
      {"/mnt/mfs/foo", "foo"},
      {"/mnt/mfs/foo/bar", "foo/bar"},
      {"/mnt/mfs/foo/", "foo/"},
    };

    for (int i=0; i<sizeof(brr)/sizeof(brr[0]); i++)
      EXPECT_EQ(std::string(brr[i].b), ToInterpath(brr[i].a)) << brr[i].a;
  }
}

TEST(Preload, Remote) {
  bool f = SetMaster("127.0.0.1:9421");
  ASSERT_TRUE(f);
  SetMountPoint("/mnt/mfs/");

  int fd = open("/mnt/mfs/test/foo", O_CREAT, 0644);
  ASSERT_TRUE(fd > 0) << " fd:" << fd << " errno:" << errno;
  EXPECT_EQ(5, write(fd, "hello", 5));

  EXPECT_EQ(5, lseek(fd, 0, SEEK_SET));
  char buf[5];
  EXPECT_EQ(5, read(fd, buf, 5));
}

// LD_PRELOAD=out/Default/lib.target/libpreloadfs.so out/Default/preload_test