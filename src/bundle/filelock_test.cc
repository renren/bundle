#include <stdarg.h>
#include "gtest/gtest.h"

#include <boost/thread.hpp>

#include "bundle/filelock.h"

const char* fn = "own";

#if defined(OS_LINUX)
TEST(FileLock, Linux) {
  int fd = creat(fn, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  ASSERT_TRUE(-1 != fd);
  close(fd);

  struct stat st;
  ASSERT_EQ(0, stat(fn, &st));

  EXPECT_EQ(-1, open(fn, O_RDWR | O_EXCL | O_CREAT 
                    , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
  // TODO: test errno

  EXPECT_EQ(0, unlink(fn));
}
#endif

TEST(FileLock, Impl) {
  using namespace bundle;
  {
    //FileLock a(fn, true);
    FileLock a(fn);
    a.TryLock();

    struct stat st;
    ASSERT_EQ(0, stat(fn, &st));
  }
  struct stat st;
  ASSERT_EQ(-1, stat(fn, &st));
}

TEST(FileLock, Test) {
	const char *lockfile[] = {
		".lock/a",
		".lock/aa",
		".lock/ab",
		".lock/aC",
		".lock/ad",
		".lock/b",
		".lock/bc",
		".lock/bd",
		".lock/c",
		".lock/ca",
		".lock/cb",
		".lock/cd",
		".lock/cc",
		".lock/d",
	};

  bundle::FileLock * filelock_a[14];
  bundle::FileLock * filelock_b[14];

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    filelock_a[i] = new bundle::FileLock(lockfile[i]);
    EXPECT_TRUE(filelock_a[i]->TryLock());
	}
	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    filelock_b[i] = new bundle::FileLock(lockfile[i]);
    EXPECT_FALSE(filelock_b[i]->TryLock());
	}

	for (int i=0; i<sizeof(lockfile)/sizeof(*lockfile); ++i) {
    delete filelock_a[i];
    delete filelock_b[i];
  }
  
  //EXPECT_EQ("1\n", Execute("find ", ".lock", " -type f | wc -l", 0));
  
}

TEST(FileLock, MultiServer) {
	const char *lockfile[] = {
		"/mnt/mfs/.locktest/a",
		"/mnt/mfs/.locktest/aa",
	};

  int num = sizeof(lockfile)/sizeof(*lockfile);
  bundle::FileLock * filelock_a[num];
  bundle::FileLock * filelock_b[num];

	for (int i=0; i<num; ++i) {
    filelock_a[i] = new bundle::FileLock(lockfile[i]);
    EXPECT_TRUE(filelock_a[i]->TryLock()) << " > index:" << i;
	}
#if 0
	for (int i=0; i<num; ++i) {
    filelock_b[i] = new bundle::FileLock(lockfile[i]);
    EXPECT_FALSE(filelock_b[i]->TryLock()) << " > index:" << i;
	}

	for (int i=0; i<num; ++i) {
    delete filelock_a[i];
    delete filelock_b[i];
  }
#endif  
  // EXPECT_EQ("1\n", Execute("find ", ".lock", " -type f | wc -l", 0));
}


volatile bool magic_ = false;
bool quit_ = false;
volatile int count_ = 0, failed_count_ = 0;

void Proc() {
  using namespace bundle;
  while(!quit_) {
    FileLock a(fn, true);
    if (a.IsLocked()) {
      EXPECT_EQ(0, magic_);
      magic_ = true;

      ++ count_;

      magic_ = false;
    }
    else
      ++ failed_count_;
  }
}

#if defined(OS_LINUX)
TEST(FileLock, Threads) {
  boost::thread_group g;
  for (int i=0; i<10; ++i)
    g.create_thread(&Proc);

  // base::Sleep(5000);
  quit_ = true;
  g.join_all();

  std::cout << "locked: " << count_ 
	    << " failed: " << failed_count_
	    << std::endl;
}
#endif

// ext3 
// locked: 91500 failed: 3147682
// wait lock
// locked: 270675 failed: 0

// moose
// try lock
// locked: 1889 failed: 27061

// wait lock
// locked: 6776 failed: 0
// delay=10ms
// locked: 6871 failed: 0
