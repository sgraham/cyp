#include "input/input.h"

#include <gtest/gtest.h>

void TestLoad(const char* input, Value* v) {
  std::string err;
  GypLoad(input, v, &err);
  EXPECT_TRUE(err.empty());
}

std::string TestErr(const char* input) {
  std::string err;
  Value v;
  GypLoad(input, &v, &err);
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

  TestLoad("{'stuff':{'things':'wee', 'waa': 'woo'}}", &v);
  EXPECT_TRUE(v.IsDict());
  EXPECT_EQ(1, v.GetDictSize());
  const Value& sub = v.GetItem("stuff");
  EXPECT_EQ(2, sub.GetDictSize());
  EXPECT_EQ("wee", sub.GetItem("things").GetString());
  EXPECT_EQ("woo", sub.GetItem("waa").GetString());
}

TEST(Parse, TrailingCommas) {
  Value v;

  TestLoad("{'stuff':'wee',}", &v);
  EXPECT_TRUE(v.IsDict());
  EXPECT_EQ(1, v.GetDictSize());

  TestLoad("[1, 2,3,]", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(3, v.GetListSize());
}

TEST(Parse, StringEscapes) {
  Value v;

  TestLoad("['\\\\', \"\\n\", '\\'', \"\\\"\"]", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(4, v.GetListSize());
  EXPECT_EQ("\\", v[0].GetString());
  EXPECT_EQ("\n", v[1].GetString());
  EXPECT_EQ("'", v[2].GetString());
  EXPECT_EQ("\"", v[3].GetString());

  TestLoad("['stuff \\\n       blah']", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(1, v.GetListSize());
  EXPECT_EQ("stuff        blah", v[0].GetString());
}

TEST(Parse, Comments) {
  Value v;

  TestLoad("# stuff\n[#hai\n'stuff'#wee\n,#xyz\n3#blah\n,]", &v);
  EXPECT_TRUE(v.IsList());
  EXPECT_EQ(2, v.GetListSize());
  EXPECT_EQ("stuff", v[0].GetString());
  EXPECT_EQ("3", v[1].GetString());

  TestLoad("# stuff\n{#hai\n'stuff'#wee\n:#xyz\n3#blah\n,}", &v);
  EXPECT_TRUE(v.IsDict());
  EXPECT_EQ(1, v.GetDictSize());
  EXPECT_EQ("3", v.GetItem("stuff").GetString());
}

// TODO: util.h
std::string ReadFile(const std::string& path) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f)
    Fatal("couldn't open '%s': %s", path.c_str(), strerror(errno));
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* data = new char[len];
  size_t read = fread(data, 1, len, f);
  if (read != len)
    Fatal("didn't read %d bytes as expected", len);
  std::string result(data, len);
  delete[] data;
  return result;
}

TEST(Parse, Large) {
  Value v;
  std::string all_gyp = ReadFile("src/input/testdata.gyp");
  TestLoad(all_gyp.c_str(), &v);
  EXPECT_TRUE(v.IsDict());
  EXPECT_EQ(4, v.GetItem("targets").GetListSize());
  EXPECT_EQ(5, v.GetItem("conditions").GetListSize());
}
