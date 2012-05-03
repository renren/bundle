#include <gtest/gtest.h>
#include "base3/pathops.h"

using namespace base;

TEST(Pathops, Dirname) {
  EXPECT_EQ("/a", Dirname("/a/b"));
  EXPECT_EQ("/a/b", Dirname("/a/b/"));
  EXPECT_EQ("a", Dirname("a/b"));
  EXPECT_EQ("", Dirname("a"));
}

TEST(Pathops, Pathjoin) {
  EXPECT_EQ("/a/b", PathJoin("/a", "b"));
  EXPECT_EQ("/a/b", PathJoin("/a/", "b"));
  EXPECT_EQ("/b", PathJoin("/a", "/b"));
  EXPECT_EQ("/a/b/", PathJoin("/a", "b/"));
  EXPECT_EQ("/b/", PathJoin("", "/b/"));
} 
