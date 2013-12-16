// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Does the equivalent of gyp.input.Load(...) and saves the result to a pickle
// file.

#include "input/input.h"
#include "input/string_piece.h"

#include <stdarg.h>

#include <future>
#include <map>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

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
  LoadCtx(const std::string& path, char* data, size_t len, Value* result);
  void Parse();

 private:
  char Peek();
  char Take();
  void SkipWhitespace();
  void GetLineAndColumn(int* line, int* column);
  NORETURN void FatalParse(const char* msg);
  void StartDict();
  void ParseDict();
  void ParseArray();
  void ParseString();
  void ParseValue();

  std::vector<Value> stack_;
  char* start_;
  char* cur_;
  char* end_;
  std::string path_;
  Value* result_;
};

LoadCtx::LoadCtx(const string& path, char* data, size_t len, Value* result)
    : start_(data),
      cur_(data),
      end_(data + len),
      path_(path),
      result_(result) {}

void LoadCtx::Parse() {
  SkipWhitespace();
  if (Peek() == '\0')
    FatalParse("text only contains white space");
  switch (Peek()) {
    case '{':
      ParseDict();
      break;
    case '[':
      ParseArray();
      break;
  }
  SkipWhitespace();
  if (Peek() != '\0')
    FatalParse("nothing should follow the root");
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
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
      Take();
    else
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

NORETURN void LoadCtx::FatalParse(const char* msg) {
  int line, column;
  GetLineAndColumn(&line, &column);
  Fatal("%s:%d:%d: %s", path_.c_str(), line, column, msg);
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
      FatalParse("name must be string");
    ParseString();
    if (Take() != ':')
      FatalParse("expected colon after object name");
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
        if (Peek() == '}')
          return;
        break;
      case '}':
        return;
    }
  }
}

void LoadCtx::ParseArray() {}

void LoadCtx::ParseString() {
  char quote = Take();
  if (quote != '\'' && quote != '"')
    FatalParse("expected ' or \" to start string");
  char* start = cur_;
  (void)start;
  for (;;) {
    char c = Take();
    if (c == quote) {
      char* end = cur_;
      (void)end;
    }
  }
}

void LoadCtx::ParseValue() {}

// Originally a restricted Python dict. Sort of json-y with # comments.
// Strings with ' or ", but no ''', etc. Top-level must be {}. Keys can be
// dicts, lists, strings (and ints, which are converted to strings). Input
// strings are decoded in place and point into contents.
//
// |contents| is modified in place (to decode string escape sequences).
void GypLoad(const string& path,
             char* data,
             size_t len,
             Value* result,
             string* err) {
  LoadCtx ctx(path, data, len, result);
  ctx.Parse();
}

