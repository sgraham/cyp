// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Does the equivalent of gyp.input.Load(...) and saves the result to a pickle
// file.

#include "string_piece.h"

#include <stdarg.h>

#include <future>
#include <map>
#include <set>
#include <string>
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

typedef map<string, string> StringDict;
typedef map<string, set<string> > LoadData;

#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN __attribute__((noreturn))
#endif

NORETURN void Fatal(const char* msg, ...) {
  va_list ap;
  fprintf(stderr, "input: fatal: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

string ReadFile(const string& path) {
  char* data = new char[4<<20];
  FILE* f = fopen(path.c_str(), "rb");
  if (!f)
    Fatal("couldn't open '%s': %s", path.c_str(), strerror(errno));
  size_t len = fread(data, 1, 1<<19, f);
  string ret(data, len);
  delete[] data;
  return ret;
}

// VS2013 doesn't yet support unrestricted unions.
struct Value {
  Value() : type(Value_Unknown), ptr(NULL) {}
  enum ValueType { Value_Unknown, Value_Dict, Value_List, Value_String };
  ValueType type;
  // TODO: Embed root of structures here.
  union {
    unordered_map<string, Value>* dict;
    vector<Value>* list;
    string* str;
    void* ptr;
  };
};

struct LoadCtx {
  LoadCtx(StringPiece contents, const char* path)
      : start_(contents.str_),
        cur_(contents.str_),
        end_(contents.str_ + contents.len_),
        path_(path) {}

  Value Parse() {
    Value ret;
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
    return ret;
  }

  char Peek() { return *cur_; }

  char Take() { return *cur_++; }

  void SkipWhitespace() {
    for (;;) {
      char c = Peek();
      if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
        Take();
      else
        break;
    }
  }

  void GetLineAndColumn(int* line, int* column) {
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

  NORETURN void FatalParse(const char* msg) {
    int line, column;
    GetLineAndColumn(&line, &column);
    Fatal("%s:%d:%d: %s", path_, line, column, msg);
  }

  void StartObject() {}

  void EndObject(size_t count) {}

  void ParseDict() {
    Take();  // Skip '{'.
    StartObject();
    SkipWhitespace();
    if (Peek() == '}') {
      // Empty dict.
      Take();
      EndObject(0);
      return;
    }
    size_t member_count = 0;
    for (;;) {
      if (Peek() != '\'')
        FatalParse("name must be string");
      ParseString();
      if (Take() != ':')
        FatalParse("expected colon after object name");
      ParseValue();
      SkipWhitespace();
      ++member_count;
      switch (Take()) {
        case ',':
          SkipWhitespace();
          // Trailing , allowed.
          if (Peek() == '}') {
            EndObject(member_count);
            return;
          }
          break;
        case '}':
          EndObject(member_count);
          return;
      }
    }
  }

  void ParseArray() {}

  void ParseString() {}

  void ParseValue() {}

 private:
  const char* start_;
  const char* cur_;
  const char* end_;
  const char* path_;
};

// Originally a restricted Python dict. Sort of json-y with # comments.
// Strings are '-delimited only. Top-level must be {}. Keys can be dicts, lists,
// strings (and ints, which are converted to strings). Input strings are not
// decoded, i.e. ascii not utf8 and resulting tree structure points into
// |source|.
Value GypDictLoad(const string& path, StringPiece source) {
  LoadCtx ctx(source, path.c_str());
  return ctx.Parse();
}

void LoadOneBuildFile(const string& build_file_path,
                      const LoadData& data,
                      void* aux_data,  // TODO: ?
                      const StringDict& variables,
                      const vector<string>& includes,
                      bool is_target) {
  // TODO: If already loaded, return already loaded.

  // Read file, or raise error.
  string build_file_contents = ReadFile(build_file_path);

  // "eval" the contents of the file, reporting syntax and evaluation errors.
  Value result = GypDictLoad(build_file_path, build_file_contents);

  // Ensure it evalutes to a dictionary.
  if (result.type != Value::Value_Dict)
    Fatal("did not evaluate to a dictionary '%s'", build_file_path.c_str());


  // TODO: Save the result into data[build_file_path]
  // TODO: Set aux_data[build_file_path] to empty dict.

  // TODO: Scan for includes and merge those into result.
  // LoadBuildFileIncludesIntoDict(...) passing includes only if is_target.

  // TODO: Return result.
}

int main() {
  LoadData data;
  string filename = "c:\\src\\cr\\src\\build\\all.gyp";
  StringDict variables;
  variables["RULE_INPUT_ROOT"] = "";
  variables["GENERATOR"] = "none";
  variables["SHARED_INTERMEDIATE_DIR"] = "dir";
  variables["RULE_INPUT_DIRNAME"] = "";
  variables["DEPTH"] = "..";
  variables["RULE_INPUT_EXT"] = "";
  variables["MSVS_VERSION"] = "2013";
  variables["EXECUTABLE_PREFIX"] = "";
  variables["PRODUCT_DIR"] = "dir";
  variables["SHARED_LIB_SUFFIX"] = "";
  variables["SHARED_LIB_DIR"] = "dir";
  variables["LIB_DIR"] = "dir";
  variables["EXECUTABLE_SUFFIX"] = "";
  variables["SHARED_LIB_PREFIX"] = "";
  variables["STATIC_LIB_PREFIX"] = "";
  variables["MSVS_OS_BITS"] = 64;
  variables["CONFIGURATION_NAME"] = "";
  variables["INTERMEDIATE_DIR"] = "dir";
  variables["RULE_INPUT_PATH"] = "";
  variables["RULE_INPUT_NAME"] = "";
  variables["STATIC_LIB_SUFFIX"] = "";
  variables["OS"] = "win";
  vector<string> includes { "c:\\src\\cr\\src\\build\\common.gypi" };
  data["target_build_files"] = set<string> { filename };
  LoadOneBuildFile(filename, data, NULL, variables, includes, true);

  return 0;
}
