#include "input/input.h"

#include <gtest/gtest.h>

#pragma warning(disable: 4996)

void TestLoad(const char* input, Value* v) {
  char* copy = strdup(input);
  std::string err;
  GypLoad(copy, strlen(copy), v, &err);
  EXPECT_TRUE(err.empty());
}

std::string TestErr(const char* input) {
  char* copy = strdup(input);
  std::string err;
  Value v;
  GypLoad(copy, strlen(copy), &v, &err);
  return err;
}

TEST(Parse, Basic) {
  Value v;

  TestLoad("{}", &v);
  EXPECT_TRUE(v.IsDict());

  EXPECT_EQ("1:1:text only contains white space", TestErr(""));
  EXPECT_EQ("2:4:text only contains white space", TestErr("  \r\t\n   "));
  EXPECT_EQ("1:3:nothing should follow the root", TestErr("{}{"));

  TestLoad("{'a':'b'}", &v);
  EXPECT_TRUE(v.IsDict());
  EXPECT_EQ(1, v.GetDictSize());
  EXPECT_TRUE(v.GetItem("a").IsString());
  EXPECT_EQ("b", v.GetItem("a").GetString());
}
