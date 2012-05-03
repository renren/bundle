#include <gtest/gtest.h>

#include <iostream>
#include "base3/asyncall.h"
#include "base3/pcount.h"
#include "base3/common.h"

#include "base3/ptime.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/detail/atomic_count.hpp>

static boost::mutex cout_mutex;

static void logpid(const char* text) {
  boost::lock_guard<boost::mutex> g(cout_mutex);
  std::cout << "[" << 0 << "] " << text << "\n";
}

static boost::detail::atomic_count* bark_count_(0);
static void bark() {
  ++ *bark_count_;
}

static int foo() {
  bark();
  return 42;
}

static int foo2(int a) {
  bark();
  return a;
}

bool stop_ = false;
boost::detail::atomic_count* post_count_(0);

void reset_count() {
  delete bark_count_;
  delete post_count_;

  bark_count_ = new boost::detail::atomic_count(0);
  post_count_ = new boost::detail::atomic_count(0);
}

void stop_post() {
  stop_ = true;
}

void once_post() {
  base::Post(&bark);
}
void much_post(int i = 100) {
  while(i--)
    base::Post(&bark);
}

void once_post_delay() {
  base::Post(&bark, 1);
}
void much_post_delay(int i = 100) {
  while(i--)
    base::Post(&bark, 1);
}

void once_post_count() {
  base::Post(&bark, 1, 1);
}
void much_post_count(int i = 100) {
  while(i--)
    base::Post(&bark, 1, 1);
}

void timed_post() {
  while (!stop_) {
    base::Post(&bark);
    ++ *post_count_;
  }
}

void parallel_post(size_t seconds, int post_thread) {
  stop_ = false;
  std::cout << "post " << seconds << " seconds in " << post_thread << " thread(s) ";

  reset_count();

  boost::asio::deadline_timer timer(base::AsyncRunner::GetService());
  timer.expires_from_now(boost::posix_time::seconds(seconds));
  timer.async_wait(boost::bind(stop_post));

  boost::thread_group g_;
  for (int i=0; i<post_thread; ++i)
    g_.add_thread(new boost::thread(&timed_post));

  g_.join_all();
  std::cout << "post " << *post_count_ << " finished " <<  *bark_count_ << "\n";
}

struct jerk {
  void foo() { ::bark();}
  int bar() { ::bark(); return 0; }
  int bark() const { ::bark(); return 0; }

  int goo(int) const { ::bark(); return 0; }
  int goo2(int, const char*) const { ::bark(); return 0; }
};

using namespace base;

TEST(AsyncallTest, Compile) {
  reset_count();
  boost::bind(&foo)();
  boost::bind(foo)();
  EXPECT_EQ(2, *bark_count_);
}

TEST(AsyncallTest, post) {
  reset_count();

  Post(&foo);
  Post(boost::bind(&foo));
  Post(boost::bind(foo));

  jerk j;
  Post(boost::bind(&jerk::foo, &j));
  Post(boost::bind(&jerk::bar, &j));
  Post(boost::bind(&jerk::bark, &j));
  Post(boost::bind(&jerk::goo, &j, 1));
  Post(boost::bind(&jerk::goo2, &j, 1, (const char*)0));

  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(8, *bark_count_);
}

TEST(AsyncallTest, delay) {
  reset_count();

  Post(&foo, 1);
  Post(boost::bind(&foo), 1);
  Post(boost::bind(foo), 1);

  jerk j;
  Post(boost::bind(&jerk::foo, &j), 1);
  Post(boost::bind(&jerk::bar, &j), 1);
  Post(boost::bind(&jerk::bark, &j), 1);
  Post(boost::bind(&jerk::goo, &j, 1), 1);
  Post(boost::bind(&jerk::goo2, &j, 1, (const char*)0), 1);

  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(8, *bark_count_);
}

TEST(AsyncallTest, count) {
  reset_count();

  Post(&foo, 1);
  Post(boost::bind(&foo), 1, 1);
  Post(boost::bind(foo), 1, 1);

  jerk j;
  Post(boost::bind(&jerk::foo, &j), 1, 1);
  Post(boost::bind(&jerk::bar, &j), 1, 1);
  Post(boost::bind(&jerk::bark, &j), 1, 1);
  Post(boost::bind(&jerk::goo, &j, 1), 1, 1);
  Post(boost::bind(&jerk::goo2, &j, 1, (const char*)0), 1, 1);

  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(8, *bark_count_);
}

TEST(AsyncallTest, cross_thread) {
  // 1
  reset_count();
  {
    boost::thread a(&once_post);
    a.join();
  }
  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(1, *bark_count_);

  reset_count();
  {
    boost::thread a(boost::bind(&much_post, 100));
    a.join();
  }
  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(100, *bark_count_);

  reset_count();
  // 2
  {
    boost::thread a(&once_post_delay);
    a.join();
  }
  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(1, *bark_count_);

  reset_count();
  {
    boost::thread a(boost::bind(&much_post_delay, 100));
    a.join();
  }
  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(100, *bark_count_);

  reset_count();
  // 3
  {
    boost::thread a(&once_post_count);
    a.join();
  }
  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(1, *bark_count_);

  reset_count();
  {
    boost::thread a(boost::bind(&much_post_count, 100));
    a.join();
  }
  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
  EXPECT_EQ(100, *bark_count_);
}

std::auto_ptr<boost::asio::io_service::work> work_;

void run3() {
  static int i_ = 0;
  if (++i_ == 3)
    work_.reset();
}

TEST(AsyncallTest, post_counted) {
  work_.reset(new boost::asio::io_service::work(base::AsyncRunner::GetService()));

  Post(&run3, 1, 3);

  base::AsyncRunner::GetService().reset();
  base::AsyncRunner::GetService().run();
}

static volatile bool _called_ = false;

static void CallMe() {
  // std::cerr << "enter callme\n";
  _called_ = true;
  // std::cerr << "leave callme\n";
}

TEST(AsyncallTest, StartStop) {
  base::Start();
  Post(&CallMe);
  Sleep(10);
  base::Stop();
  // std::cerr << "before check\n";
  EXPECT_TRUE(_called_);
  
  _called_ = false;
  base::Start();
  Post(&CallMe);
  base::Sleep(10);
  base::Stop();
  EXPECT_TRUE(_called_);
}

#if 0
TEST(AsyncallTest, sure) {
  // TODO:
}
#endif

TEST(AsyncallTest, timed_parallel_post) {
  boost::asio::io_service::work work(base::AsyncRunner::GetService());
  base::AsyncRunner::instance().Start();
  
  parallel_post(1, 1);
  parallel_post(5, 2);
  parallel_post(10, 2);
}
