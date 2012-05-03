#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/detail/atomic_count.hpp>
#include <gtest/gtest.h>

static boost::mutex cout_mutex;

static void logpid(const char* text) {
  boost::lock_guard<boost::mutex> g(cout_mutex);
  std::cout << "[" << 0 << "] " << text << "\n";
}

boost::thread_group g_;
boost::asio::io_service io_;

static void my_poll() {
  std::cout << "my_poll before\n";
  size_t c = io_.run_one();
  std::cout << "my_poll end " << c << "\n";
}

static void foo() {
  std::cout << "foo\n";
}

TEST(AsyncCall, Second) {
  boost::thread* p = new boost::thread(&foo);
#ifdef OS_LINUX
  sleep(1);
#endif
  p->join();
  delete p;

#if 1
  io_.post(&foo);
  io_.post(&foo);
#else
    boost::asio::deadline_timer timer(io_, boost::posix_time::seconds(1));
    timer.async_wait(boost::bind(&foo));
#endif

#if 0
    for (int i=0; i<2; ++i)
      g_.add_thread(new boost::thread(&my_poll));

    g_.add_thread(new boost::thread(
      boost::bind(
        &boost::asio::io_service::run, &AsyncRunner::GetService()
      )));
#endif

  boost::asio::io_service::work work(io_);
    

  g_.join_all();
}
