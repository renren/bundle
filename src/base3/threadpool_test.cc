#include <gtest/gtest.h>

#include <boost/detail/atomic_count.hpp>
#include <boost/bind.hpp>
#include <algorithm>

#include "base3/common.h" // for sleep
#include "base3/logging.h"
#include "base3/threadpool.h"

using namespace base;

static boost::detail::atomic_count sleep_count_(0);
const int test_count = 100000;

int rand_int(int c) {
  return rand() % c;
}

class SleepMessage : public Message {
public:
  SleepMessage(int millisec = 1) : millisec_(millisec)
  {}

  virtual void Run() {
    base::Sleep(millisec_);
    ++ sleep_count_;
  }

private:
  int millisec_;
};

int loop_count = 0;

void ThreadPost(base::ThreadPool* mq) {
  int c = test_count;
  while (c > 0) {
    boost::thread::yield();

    const int rc = std::min(c, rand_int(50));

    std::vector<base::Message*> arr(rc);
    for (int i=0; i<rc; ++i)
      arr[i] = new SleepMessage;
    mq->Post(arr.begin(), arr.end());

    c -= rc;
    loop_count += rc;
  }
  LOG(INFO) << "quit post " << c;
}

#if 0
TEST(WithThreadPool, HowMuch) {
  ThreadPool pool(10, 500);

  boost::thread bthread(boost::bind(&ThreadPost, &pool));
  
  LogMessage::LogToDebug(LS_VERBOSE);

  while (sleep_count_ < test_count) {
    Sleep(1000);
    LOG(INFO) << "processed " << sleep_count_ 
      << " posted " << pool.message_count()
      << " looped " << loop_count;
  }

  EXPECT_EQ(sleep_count_, test_count);
  bthread.join();
  pool.Stop();
  pool.Join();
}

TEST(WithThreadPool, Shrunk) {
  ThreadPool pool(10, 100);
  EXPECT_EQ(10, pool.size());

  pool.set_size(1);
}
#endif