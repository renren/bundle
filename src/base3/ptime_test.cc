#include <gtest/gtest.h>
#include <math.h>
#include <boost/thread/xtime.hpp> // delay
#include <boost/thread.hpp>
#include "base3/logging.h"
#include "base3/ptime.h"


void foo(int c = 10000000) {
  double d = c;
  while (d-- > 0)
   sin(d/c * 3.14 / 180);
}


TEST(PtimeTest, LogTrueOfFalse) {
  {
    base::ptime pt("sleep(1)", true, 0);
    // Sleep(1);

    pt.check("foo(100)");
    foo(100);
  }

  PTIME(ptm, "foo(big)", true, 0);
  foo(10000000);
  PTIME_CHECK(ptm, "foo(small)");
  foo(10000);
}

TEST(ptimeTest, Macro) {
  using namespace base;
  // LogMessage::LogToDebug(LS_INFO);
  PTIME(ptm, "pt_log_true_1ms", false, 1);
  foo();
}
