#include <gtest/gtest.h>
#include <boost/detail/atomic_count.hpp>

#include "base3/logrotate.h"

#if 0

TEST(LogRotate, compile) {
  using namespace base;

  detail::Async::Start();

  LogRotate& r = LogRotate::instance();
  r.Start("test", LS_INFO);

  LOG(INFO) << "rotate 1st";
  r.Rotate();
  
  LOG(INFO) << "rotate 2nd";
  r.Rotate();

  LOG(INFO) << "rotate 3rd";
  r.Rotate();

  LOG(INFO) << "rotate 4th";
  r.Rotate();
}

TEST(LogRotate, Open) {
  using namespace base;

  LogRotate& r = LogRotate::instance();
  r.Start("test", LS_INFO);
  r.Close();

  r.Start("/data/a", LS_INFO);
  r.Close();

  r.Start("../a/test", LS_INFO);
  r.Close();

  boost::posix_time::ptime pt = LogRotate::NextTime();
  std::cout << "next: " << pt << std::endl;
}

bool quit_  = false;

void log() {
  std::cout << "done now: " << boost::posix_time::second_clock::local_time() << std::endl;
}
#endif

#if 0
TEST(LogRotate, Expire) {
  using namespace boost::gregorian;
  // using namespace boost::posix_time;

  boost::posix_time::ptime tomorrow;
  // ugly, TODO: better impl
  boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
  date today = now.date(); // ????
  tomorrow = now + boost::posix_time::seconds(3); // 4:00

  boost::asio::io_service io;
  // boost::posix_time::second_clock
  boost::asio::deadline_timer timer(io, boost::posix_time::second_clock::local_time()
    + boost::posix_time::seconds(3));

  std::cout << "now: " << now << std::endl;
  std::cout << "expect: " << tomorrow << std::endl;

  timer.wait();
  std::cout << "done now: " << boost::posix_time::second_clock::local_time() << std::endl;
}
#endif
