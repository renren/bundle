#include <gtest/gtest.h>

#include "base3/jdbcurl.h"

using namespace base;


TEST(JdbcTest, Parse) {
  {
    const char* j = "jdbc:mysql://host,failoverhost:3001/database";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "host");
    EXPECT_EQ(jd.failoverhost, "failoverhost");
    EXPECT_EQ(jd.port, 3001);
    EXPECT_EQ(jd.database, "database");
    EXPECT_TRUE(jd.properties.empty());
  }

  {
    const char* j = "jdbc:mysql://host,failoverhost:3001/database?a=b";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "host");
    EXPECT_EQ(jd.failoverhost, "failoverhost");
    EXPECT_EQ(jd.port, 3001);
    EXPECT_EQ(jd.database, "database");
    EXPECT_EQ(jd.properties.front().first, "a");
    EXPECT_EQ(jd.properties.front().second, "b");
  }

  {
    const char* j = "jdbc:mysql://host,failoverhost:3001/database?a=b&d=e";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "host");
    EXPECT_EQ(jd.failoverhost, "failoverhost");
    EXPECT_EQ(jd.port, 3001);
    EXPECT_EQ(jd.database, "database");
    EXPECT_EQ(jd.properties.front().first, "a");
    EXPECT_EQ(jd.properties.front().second, "b");
    EXPECT_EQ(jd.properties.back().first, "d");
    EXPECT_EQ(jd.properties.back().second, "e");
  }

  {
    const char* j = "jdbc:mysql://host,failoverhost:3001/database?a=b&d";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "host");
    EXPECT_EQ(jd.failoverhost, "failoverhost");
    EXPECT_EQ(jd.port, 3001);
    EXPECT_EQ(jd.database, "database");
    EXPECT_EQ(jd.properties.front().first, "a");
    EXPECT_EQ(jd.properties.front().second, "b");
    EXPECT_EQ(jd.properties.back().first, "d");
    EXPECT_EQ(jd.properties.back().second, "");
  }

  {
    const char* j = "jdbc:mysql://host,failoverhost:3001/database?user=b&password=cwf";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "host");
    EXPECT_EQ(jd.failoverhost, "failoverhost");
    EXPECT_EQ(jd.port, 3001);
    EXPECT_EQ(jd.database, "database");
    EXPECT_EQ(jd.user, "b");
    EXPECT_EQ(jd.passwd, "cwf");
  }

  {
    const char* j = "jdbc:mysql://h/database";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "h");
    EXPECT_EQ(jd.failoverhost, "");
    EXPECT_EQ(jd.port, 3301);
    EXPECT_EQ(jd.database, "database");
  }

  {
    const char* j = "jdbc:mysql://h,f/database";
    JdbcDriver jd;
    EXPECT_TRUE(base::Parse(j, &jd));
    EXPECT_EQ(jd.host, "h");
    EXPECT_EQ(jd.failoverhost, "f");
    EXPECT_EQ(jd.port, 3301);
    EXPECT_EQ(jd.database, "database");
  }

  {
    const char* j = "Jdbc:mysql://h,f/database";
    JdbcDriver jd;
    EXPECT_TRUE(!base::Parse(j, &jd));
  }
  {
    const char* j = "jdbc:mysql://h:0/database";
    JdbcDriver jd;
    EXPECT_TRUE(!base::Parse(j, &jd));
  }
}
