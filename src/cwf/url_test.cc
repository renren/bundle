#include <gtest/gtest.h>

#include "cwf/stream.h"

using namespace cwf;

bool CheckResult(const char* q, const StringMap& m) {
  StringMap parsed = Request::ParseQuery(q);
  EXPECT_TRUE(parsed.size() == m.size());
  // BOOST_CHECK_EQUAL(parsed, m);
  for (StringMap::const_iterator i=m.begin(), j=parsed.begin();
    i!=m.end(); ++i, ++j) {
      EXPECT_EQ(i->first, j->first);
      EXPECT_EQ(i->second, j->second);
  }
  return true;
}

struct string_pair {
  const char* k;
  const char* v;
};

StringMap BuildMap(const string_pair* arr, size_t arr_length) {
  StringMap m;
  for (size_t i=0; i<arr_length; ++i) {
    m.insert(std::make_pair(arr[i].k, arr[i].v));
  }
  return m;
}

TEST(URL, Parse) {
  // http://www.google.cn/search?hl=zh-CN&source=hp&q=%E4%B8%AD%E6%96%87&aq=f&oq=
  // http://www.google.cn/search?q=%E4%B8%AD%E6%96%87&hl=zh-CN&tbo=1&newwindow=1&tbs=frm:1,sbd:1

  const char* input[] = {
    "hl=zh-CN&source=hp&q=%E4%B8%AD%E6%96%87&aq=f&oq=",
    "q=%E4%B8%AD%E6%96%87&hl=zh-CN&tbo=1&newwindow=1&tbs=frm:1,sbd:1"
  };

  const string_pair r1[] = {
    {"hl", "zh-CN"},
    {"source", "hp"},
    {"q", "中文"},
    {"aq", "f"},
    {"oq", ""},
  };
    
  const string_pair r2[] = {
    {"hl", "zh-CN"},
    {"tbo", "1"},
    {"q", "中文"},
    {"newwindow", "1"},
    {"tbs", "frm:1,sbd:1"},
  };
  
  CheckResult(input[0], BuildMap(r1, ARRAY_SIZE(r1)));
  CheckResult(input[1], BuildMap(r2, ARRAY_SIZE(r2)));
}
