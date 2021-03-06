// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Does the equivalent of gyp.input.Load(...) and saves the result to a pickle
// file.

#include "input/input.h"
#include "input/string_piece.h"

#include <stdarg.h>
#include <setjmp.h>

#include <future>
#include <map>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable: 4127)  // Conditional expression is constant.
#pragma warning(disable: 4324)  // Structure was padded due to __declspec.
#endif

using std::async;
using std::future;
using std::map;
using std::set;
using std::string;
using std::vector;
using std::unordered_map;

NORETURN void Fatal(const char* msg, ...) {
  va_list ap;
  fprintf(stderr, "input: fatal: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

class LoadCtx {
 public:
  LoadCtx(const std::string& data, Value* result, string* err);
  void Parse();

 private:
  char Peek();
  char Take();
  void SkipWhitespace();
  void GetLineAndColumn(int* line, int* column);
  void StartDict();
  void ParseDict();
  void ParseList();
  void ParseString();
  void ParseNumber();
  void ParseValue();

  std::vector<Value> stack_;
  const char* start_;
  const char* cur_;
  const char* end_;
  Value* result_;

  jmp_buf jmpbuf_;
  std::string* err_;
};

#define PARSE_ERROR(msg)                                        \
  do {                                                          \
    int line, column;                                           \
    GetLineAndColumn(&line, &column);                           \
    char buf[256];                                              \
    sprintf_s(buf, sizeof(buf), "%d:%d:%s", line, column, msg); \
    *err_ = buf;                                                \
    longjmp(jmpbuf_, 1);                                        \
  } while (0)

LoadCtx::LoadCtx(const std::string& data, Value* result, string* err)
    : start_(data.c_str()),
      cur_(data.c_str()),
      end_(data.c_str() + data.size()),
      result_(result),
      err_(err) {
  result_->SetNone();
  err_->clear();
}

void LoadCtx::Parse() {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4611)  // interaction between '_setjmp' and C++ \
                                 // object destruction is non-portable
#endif
  if (setjmp(jmpbuf_)) {
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // err_ will be set.
    result_->SetNone();
    stack_.clear();
    return;
  }

  SkipWhitespace();
  if (Peek() == 0)
    PARSE_ERROR("text only contains white space");
  switch (Peek()) {
    case '{':
      ParseDict();
      break;
    case '[':
      ParseList();
      break;
  }
  SkipWhitespace();
  if (Peek() != 0)
    PARSE_ERROR("nothing should follow the root");
  *result_ = stack_.back();
  stack_.pop_back();
}

char LoadCtx::Peek() {
  if (cur_ == end_)
    return 0;
  return *cur_;
}

char LoadCtx::Take() {
  if (cur_ == end_)
    return 0;
  return *cur_++;
}

void LoadCtx::SkipWhitespace() {
  for (;;) {
    char c = Peek();
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      Take();
    } else if (c == '#') {
      Take();
      for (;;) {
        c = Peek();
        if (c == 0)
          return;
        if (c == '\n')
          break;
        Take();
      }
    } else
      break;
  }
}

void LoadCtx::GetLineAndColumn(int* line, int* column) {
  *line = 1;
  *column = 1;
  for (const char* p = start_; p != cur_; ++p) {
    if (*p == '\r')
      continue;
    if (*p == '\n') {
      *line += 1;
      *column = 1;
      continue;
    }
    *column += 1;
  }
}

void LoadCtx::ParseDict() {
  Take();  // Skip '{'.

  Value v;
  v.SetDict();
  stack_.push_back(v);

  SkipWhitespace();
  if (Peek() == '}') {
    // Empty dict.
    Take();
    return;
  }
  for (;;) {
    if (Peek() != '\'' && Peek() != '"')
      PARSE_ERROR("name must be string");
    ParseString();
    SkipWhitespace();
    if (Take() != ':') {
      cur_--;
      PARSE_ERROR("expected colon after name");
    }
    SkipWhitespace();
    ParseValue();
    SkipWhitespace();
    Value& dict = stack_.at(stack_.size() - 3);
    const Value& key = stack_.at(stack_.size() - 2);
    const Value& value = stack_.at(stack_.size() - 1);
    dict.SetItem(key.GetString(), value);
    stack_.pop_back();
    stack_.pop_back();
    switch (Take()) {
      case ',':
        SkipWhitespace();
        // Trailing , allowed.
        if (Peek() == '}') {
          Take();
          return;
        }
        break;
      case '}':
        return;
    }
  }
}

void LoadCtx::ParseList() {
  Take();  // Skip '['.

  Value v;
  v.SetList();
  stack_.push_back(v);

  SkipWhitespace();
  if (Peek() == ']') {
    // Empty list.
    Take();
    return;
  }
  for (;;) {
    ParseValue();
    Value& list = stack_.at(stack_.size() - 2);
    const Value& value = stack_.at(stack_.size() - 1);
    stack_.pop_back();
    list.ListPushBack(value);
    SkipWhitespace();
    switch (Take()) {
      case ',':
        SkipWhitespace();
        // Trailing , allowed.
        if (Peek() == ']') {
          Take();
          return;
        }
        break;
      case ']':
        return;
    }
  }
}

void LoadCtx::ParseString() {
  char quote = Take();
  if (quote != '\'' && quote != '"')
    PARSE_ERROR("expected ' or \" to start string");

  Value v;
  stack_.push_back(v);

  string result;
  for (;;) {
    char c = Take();
    if (c == 0)
      PARSE_ERROR("EOF while parsing string");  // TODO: Add start location.
    if (c == quote) {
      stack_.back().SetString(result);
      return;
    } else if (c == '\\') {
      switch (Take()) {
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case '\\':
          result.push_back('\\');
          break;
        case '\'':
          result.push_back('\'');
          break;
        case '"':
          result.push_back('"');
          break;
        case '\n':
          // Line continuation, ignored.
          break;
        default:
          assert(false && "todo; unhandled escape");
          break;
      }
    } else {
      result.push_back(c);
    }
  }
}

void LoadCtx::ParseNumber() {
  // TODO: Negative numbers?
  if (Peek() < '0' || Peek() > '9')
    PARSE_ERROR("value expected");
  int i = 0;
  while (Peek() >= '0' && Peek() <= '9')
    i = i * 10 + (Take() - '0');
  char buf[32];
  sprintf(buf, "%d", i);
  Value v;
  v.SetString(buf);
  stack_.push_back(v);
}

void LoadCtx::ParseValue() {
  switch (Peek()) {
    case '"':
    case '\'':
      ParseString();
      break;
    case '{':
      ParseDict();
      break;
    case '[':
      ParseList();
      break;
    default:
      ParseNumber();
      break;
  }
}

// Originally a restricted Python dict. Sort of json-y with # comments.
// Strings with ' or ", but no ''', etc. Top-level must be {}. Keys can be
// dicts, lists, strings (and ints, which are converted to strings). Input
// strings are decoded in place and point into contents.
void GypLoad(const std::string& data, Value* result, string* err) {
  LoadCtx ctx(data, result, err);
  ctx.Parse();
}

