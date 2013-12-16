#include "input/input.h"

#include <gtest/gtest.h>

#pragma warning(disable: 4996)

void TestLoad(const char* input, Value* v) {
  char* copy = strdup(input);
  std::string err;
  GypLoad("test.gyp", copy, strlen(copy), v, &err);
  EXPECT_TRUE(err.empty());
}

TEST(Parse, Basic) {
  Value v;

  TestLoad("{}", &v);
  EXPECT_TRUE(v.IsDict());

  //TestFail("{}{", &v);
  //TestFail("{'a','b'}", &v);

  TestLoad("{'a':'b'}", &v);
  EXPECT_TRUE(v.IsDict());
  EXPECT_EQ(1, v.GetDictSize());
  EXPECT_TRUE(v.GetItem("a").IsString());
  //EXPECT_EQ("b", v.GetItem("a").GetString());
}
