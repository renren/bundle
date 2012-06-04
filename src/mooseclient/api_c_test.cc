#include "gtest/gtest.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#ifdef OS_WIN
  #include <winsock2.h>
  #define snprintf _snprintf
#endif

#include "mooseclient/moose_c.h"

TEST(CAPI, Example) {
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

std::string FakeString(int size = 100, char ch = 'a') {
  std::string content(size, ch);
  for (int j=39; j<content.size(); j+=40)
    content[j] = '\n';

  content[content.size() - 1] = '\n';
  return content;
}

TEST(CAPI, HugeWrite) {
  const char* fn = "test/hugewrite.txt";

  const int SIZE_TO_WRITE = 1024*1024*8 - 1;

  std::string content = FakeString(SIZE_TO_WRITE);

  int fd = mfs_open(fn, O_CREAT|O_RDWR|O_APPEND, 0644);
  ASSERT_TRUE(fd > 0) << " fd:" << fd << " errno:" << errno;

  int a = mfs_write(fd, content.data(), content.size());
  EXPECT_EQ(content.size(), a) << "return " << a;

  EXPECT_EQ(0, mfs_close(fd));
}

TEST(CAPI, MultiWrite) {
  const int M = 1024 * 1024;
  int arr[] = {
    1, 10, 100, 1023, 1024, 1025 // a b c d e
    , 1024*4, 1024*4+1, 1024*1024, 1024*1024 + 1, 1024*1024 + 100
    , M * 2, M * 2 + 1, M * 3 -1, M * 4, M * 5    // k l m n o
    , M * 6, M * 7 + 1, M * 8 -1, M * 9, M * 10   // p q r s t
    , 1024*1024*64-1, 1024*1024*64, 1024*1024*64+1
  };

  const char* fn = "test/multiwrite.txt";

  mfs_unlink(fn);

  for (int i=0; i<sizeof(arr)/sizeof(*arr); ++i) {
    int fd = mfs_open(fn, O_CREAT|O_RDWR|O_APPEND, 0644);
    ASSERT_TRUE(fd > 0) << " fd:" << fd << " errno:" << errno;

    std::string content = std::string(arr[i], 'a' + i);
    content.append("\n");
    for (int j=39; j<content.size(); j+=40)
      content[j] = '\n';

    int a = mfs_write(fd, content.data(), content.size());
    EXPECT_EQ(content.size(), a) << "return " << a;

    EXPECT_EQ(0, mfs_close(fd));
  }
}

TEST(CAPI, MultiWrite2) {
  const char* fn = "test/multiwrite2.txt";

  const int SIZE_TO_WRITE = 1024*1024*5;

  int c = 1000;
  while (c --) {
    std::string content = FakeString(rand() % SIZE_TO_WRITE, 'a' + c % 26);

    int fd = mfs_open(fn, O_CREAT|O_RDWR|O_APPEND, 0644);
    ASSERT_TRUE(fd > 0) << " fd:" << fd << " errno:" << errno;

    int a = mfs_write(fd, content.data(), content.size());
    EXPECT_EQ(content.size(), a) << "return " << a;

    EXPECT_EQ(0, mfs_close(fd));
  }
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

int main(int argc, char **argv) {
  std::cout << "Running main() from mooseclient/api_c_test.cc\n";

#ifdef WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
#endif

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
