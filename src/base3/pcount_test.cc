#include <iostream>
#include <gtest/gtest.h>
#include <boost/bind.hpp>

#if 0
#include "base3/pcount.h"

using base::pcount;

class TaskManager {
public:
  TaskManager() : XAR_INIT(task) {}
  void ExecuteOneTask() {
    XAR_INC(task);
  }

  void FinishedOneTask() {
    XAR_DEC(task);
  }

  size_t size() const {
    return 1;
  }

  size_t size2() {
    return 2;
  }

private:
  XAR_DECLARE(task);
};

size_t static_size() {
  return 42;
}



TEST(PcountTest, Pcount_) {
  base::pcount& p = base::pcount::instance();
  size_t index_a = p.push_add("a", 3);
  // EXPECT_EQ(index_a, 0);

  size_t index_b = p.push_add("b", 3);
  EXPECT_EQ(index_b, index_a + 1);

  p.push_title();
  std::cout << std::endl;

  TaskManager tm;
  tm.ExecuteOneTask();
  tm.FinishedOneTask();

  p.push_stat();
  std::cout << std::endl;

  tm.ExecuteOneTask();
  tm.ExecuteOneTask();
  tm.ExecuteOneTask();
  p.push_stat();

  std::cout << std::endl;
  std::cout << std::endl;
}

TEST(PcountTest, Push) {
  base::pcount& p = base::pcount::instance();
  TaskManager tm;

  p.poll_add("const_member", boost::bind(&TaskManager::size, &tm));
  p.poll_add("member", boost::bind(&TaskManager::size2, &tm));
  

  // base::xar::instance().output(std::cout);
  p.poll_title();
  std::cout << std::endl;
  p.poll_stat();
  std::cout << std::endl;

  p.poll_add("static_size", boost::bind(static_size));

  p.poll_title();
  std::cout << std::endl;
  p.poll_stat();

  std::cout << std::endl;
}

TEST(PcountTest, Rusage) {
  base::pcount& p = base::pcount::instance();
  base::xar& x = base::xar::instance();

  std::cout << "-- push title --\n";
  std::cout << p.push_title();

  std::cout << "\n-- poll title --\n";
  std::cout << p.poll_title();

  std::cout << "\n-- rusage title --\n";
  std::cout << x.rusage_title();
  
  // std::cout << "\n-- rusage title --\n";
  // std::cout << "title: " << x.rusage_title();
  
   std::cout << "\n-- rusage --\n";
  std::cout << "       " << x.rusage();

  std::cout << "\n-- all --\n";
  x.output(std::cout);
}
#endif