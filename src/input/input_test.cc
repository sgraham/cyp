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

  TestLoad("[]", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(0, v.GetListSize());

  TestLoad("['a']", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(1, v.GetListSize());
  EXPECT_EQ("a", v[0].GetString());

  TestLoad("[1, \"stuff\", 43534]", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(3, v.GetListSize());
  EXPECT_EQ("1", v[0].GetString());
  EXPECT_EQ("stuff", v[1].GetString());
  EXPECT_EQ("43534", v[2].GetString());
}

TEST(Parse, VariousErrors) {
  EXPECT_EQ("1:2:name must be string", TestErr("{[]:'hi'}"));
  EXPECT_EQ("1:5:expected colon after name", TestErr("{'a','hi'}"));
  EXPECT_EQ("1:6:EOF while parsing string", TestErr("['abc"));
  EXPECT_EQ("1:4:value expected", TestErr("[[]"));
}

TEST(Parse, Nested) {
  Value v;
}

TEST(Parse, StringEscapes) {
  Value v;
}
