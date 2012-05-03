#include <gtest/gtest.h>
#include <iostream>
#include <boost/thread.hpp>

#include "base3/rwlock.h"
#include "base3/common.h"

static bool quit_ = false;
unsigned count = 0;
base::ReadWriteLock lock;

void ReaderProc() {
  while(!quit_) {
    lock.ReadLock();
    // std::cout << "cnt = " << count << std::endl;
    lock.ReadUnlock();
    base::Sleep(50);
  }
}

void WriterProc() {
  while(!quit_) {
    lock.WriteLock();
    std::cout << "writing count to " << ++ count << std::endl;
    base::Sleep(1);
    lock.WriteUnlock();

    base::Sleep(100);
  }
}

TEST(ReadwriteLockTest, Stop) {
  boost::thread_group g;

  for (int i = 0; i < 20; ++i)
    g.create_thread(&ReaderProc);

  g.create_thread(&WriterProc);
  g.create_thread(&WriterProc);

  base::Sleep(2000);
  quit_ = true;

  g.join_all();
}
