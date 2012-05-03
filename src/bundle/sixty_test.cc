#include <gtest/gtest.h>
#include <iomanip>
#include <limits>

#include <boost/lexical_cast.hpp>

#include "bundle/sixty.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof((x))/sizeof(*(x)))
#endif

using namespace bundle;

TEST(Sixty, Unsigned) {
	int a = -1;
	uint32_t u = a;
	
	EXPECT_EQ("4294967295", boost::lexical_cast<std::string>(u));
	int b = u;
	EXPECT_EQ("-1", boost::lexical_cast<std::string>(b));
}

TEST(Sixty, Convert) {
	EXPECT_EQ("", ToSixty(-1));
	EXPECT_EQ("", ToSixty(-2));
	EXPECT_EQ("A", ToSixty(0));

	EXPECT_EQ(-1, FromSixty(""));

	EXPECT_EQ("", ToSixty(std::numeric_limits<int64_t>::min()));
	EXPECT_EQ("", ToSixty(std::numeric_limits<int64_t>::min() + 1));
	

  int64_t arr[] = {
		  std::numeric_limits<int64_t>::max() - 2
		, std::numeric_limits<int64_t>::max() - 1
		, std::numeric_limits<int64_t>::max()

    , 1, 10, 11, 30, 88, 99, 100, 101, 200, 400, 9999, 1000, 10001
    , 312312, 901238, 238432974
		, 783467234u, 3948475566u
  };
  for (int i=0; i<ARRAY_SIZE(arr); ++i) {
    std::string a = ToSixty(arr[i]);
#if 0
    std::cout << std::setw(14) << arr[i] 
      << std::setw(14) << a << std::endl;
#endif
    EXPECT_LE(a.size(), boost::lexical_cast<std::string>(arr[i]).size());
    EXPECT_EQ(arr[i], FromSixty(a));
  }

  // big than int64_t
  EXPECT_EQ(-1, FromSixty("2394u234nkasdfasdfas234n2k34n23k4b"));
  // maxed value:QQOkhg5VQfH plus 1 QQOkhg5VQfJ
  EXPECT_EQ(-1, FromSixty("QQOkhg5VQfJ"));
  // equal to int64_t max
  EXPECT_EQ(9223372036854775807, FromSixty("QQOkhg5VQfH"));
  // minus 1
  EXPECT_EQ(9223372036854775806, FromSixty("QQOkhg5VQfG"));
  // negative
  EXPECT_STREQ("", ToSixty(-293409234).c_str());
}

TEST(Sixty, Profiler) {
  const int test_count = 100 * 1024;
  {
    for (int i=0; i<test_count; ++i)
      ToSixty(i);
  }

  std::vector<std::string> vs;
  vs.reserve(test_count);
  for (int i=0; i<test_count; ++i) {
    vs.push_back(ToSixty(i));
  }

  {
    for (std::vector<std::string>::const_iterator i=vs.begin();
      i != vs.end(); ++i)
      FromSixty(*i);
  }
}
