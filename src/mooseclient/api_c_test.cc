#include "gtest/gtest.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "mooseclient/moose_c.h"

void ConnectOnce() {
  static bool f = false;
  if (!f) {
    f = mfs_connect("127.0.0.1:9421");
  }
}

TEST(CAPI, Example) {
  ConnectOnce();

  int fd = mfs_open("test/foo", O_CREAT, 0644);
  ASSERT_TRUE(fd > 0) << " fd:" << fd << " errno:" << errno;

  char sz[16];
  snprintf(sz, sizeof(sz), "%d", time(0));
  int a = strlen(sz);
  EXPECT_EQ(a, mfs_write(fd, sz, a));

  EXPECT_EQ(a, mfs_lseek(fd, 0, SEEK_SET));

  char buf[16];
  EXPECT_EQ(a, mfs_read(fd, buf, sizeof(buf)));

  EXPECT_EQ(a, mfs_lseek(fd, 1, SEEK_SET));
  EXPECT_EQ(a-1, mfs_read(fd, buf, sizeof(buf)));

  EXPECT_EQ(0, mfs_close(fd));
}

TEST(CAPI, Write) {
  ConnectOnce();

  int fd = mfs_open("test/write.txt", O_CREAT, 0644);
  ASSERT_TRUE(fd > 0) << " fd:" << fd << " errno:" << errno;

  int expect_size = 0;

  {
    char sz[16];
    snprintf(sz, sizeof(sz), "%d\n", time(0));
    int a = strlen(sz);
    EXPECT_EQ(a, mfs_write(fd, sz, a));
    expect_size += a;
  }

  {
    char sz[16];
    snprintf(sz, sizeof(sz), "%d\n", time(0));
    int a = strlen(sz);
    EXPECT_EQ(a, mfs_write(fd, sz, a));
    expect_size += a;
  }

  {
    EXPECT_EQ(expect_size, mfs_lseek(fd, 0, SEEK_SET));

    char *buf = new char[expect_size/2];
    EXPECT_EQ(expect_size/2, mfs_read(fd, buf, expect_size/2)) << buf;

    EXPECT_EQ(expect_size/2, mfs_read(fd, buf, expect_size/2));
    delete [] buf;

    // EXPECT_EQ(expect_size, mfs_tell(fd, buf, expect_size));
  }
  EXPECT_EQ(0, mfs_close(fd));
}

TEST(CAPI, Unlink) {
  EXPECT_TRUE(mfs_creat("own", 0644) > 0);
  EXPECT_TRUE(mfs_unlink("own") == 0);
}

TEST(CAPI, Stat) {
  const char *p = "test/p/20120512/000000fa/0000006c";

  mfs_unlink(p);
  EXPECT_TRUE(0 != mfs_stat(p, 0));

  p = "own";
  mfs_unlink(p);
  EXPECT_TRUE(0 != mfs_stat(p, 0));

  EXPECT_TRUE(mfs_creat("own", 0644) > 0);
  EXPECT_TRUE(0 == mfs_stat(p, 0));

  mfs_unlink(p);
  EXPECT_TRUE(0 != mfs_stat(p, 0));
}
