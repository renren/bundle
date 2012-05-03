#include <gtest/gtest.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/detail/atomic_count.hpp>

#include "base3/common.h" // for sleep
#include "base3/messagequeue.h"

static boost::detail::atomic_count sleep_count_(0);
const int message_count = 10;

using namespace base;

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

void ProcessProc(MessageQueue* mq) {
  mq->Process();
}

void PostProc(MessageQueue* mq) {
  int c = message_count;
  while (c--) {
    base::Sleep(1);
    mq->Post(new SleepMessage);
  }

  base::Sleep(1);
  mq->Close();
}

TEST(WithMessageQueue, Properties) {
  MessageQueue *q = new MessageQueue();
  EXPECT_TRUE(0 == q->processed());
  EXPECT_TRUE(0 == q->size());
  EXPECT_TRUE(0 == q->busy());

  q->Post(new Message());
  EXPECT_TRUE(0 == q->processed());
  EXPECT_TRUE(1 == q->size());
  EXPECT_TRUE(0 == q->busy());

  boost::thread athread(boost::bind(&ProcessProc, q));
  Sleep(3);
  q->Close();
  athread.join();
  EXPECT_TRUE(1 == q->processed());
  EXPECT_TRUE(0 == q->size());
  EXPECT_TRUE(0 == q->busy());
  delete q;

  // busy
  q = new MessageQueue();
  q->Post(new SleepMessage(10));
  EXPECT_TRUE(0 == q->processed());
  EXPECT_TRUE(1 == q->size());
  EXPECT_TRUE(0 == q->busy());
  athread = boost::thread(boost::bind(&ProcessProc, q));
  Sleep(1);
  EXPECT_TRUE(1 == q->busy());
  q->Close();
  athread.join();
  EXPECT_TRUE(1 == q->processed());
  EXPECT_TRUE(0 == q->size());
  EXPECT_TRUE(0 == q->busy());
  delete q;
}

TEST(WithMessageQueue, Post) {
  MessageQueue *q = new MessageQueue();
  Message* arr[3] = {0};
  q->Post(&arr[0], &arr[3]);
  EXPECT_TRUE(3 == q->size());

  std::vector<Message*> vec;
  q->Post(vec.begin(), vec.end());
  EXPECT_TRUE(3 == q->size());

  std::list<Message*> al;
  al.push_back(new Message());
  al.push_back(new Message());
  q->Post(al.begin(), al.end());
  EXPECT_TRUE(5 == q->size());
  delete q;
}

TEST(WithMessageQueue, Close) {
  MessageQueue *q = new MessageQueue();
  boost::thread athread(boost::bind(&ProcessProc, q));
  q->Close();
  athread.join();
  delete q;
}

TEST(WithMessageQueue, Execute) {
  MessageQueue q;
  {
    boost::thread athread(boost::bind(&ProcessProc, &q));
    boost::thread bthread(boost::bind(&PostProc, &q));

    bthread.join();
    athread.join();
  }

  // EXPECT_EQ(sleep_count_, message_count + 2);
}
