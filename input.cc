// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Does the equivalent of gyp.input.Load(...) and saves the result to a pickle
// file.

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

// VS2013 doesn't have unrestricted unions yet.
struct Value {
  Value() : type(Value_Unknown), ptr(NULL) {}
  enum ValueType { Value_Unknown, Value_Dict, Value_List, Value_String };
  ValueType type;
  union {
    unordered_map<string, Value>* dict;
    vector<Value>* list;
    string* str;
    void* ptr;
  };
};

// Originally a restricted Python dict. Sort of json-y with # comments.
// Strings are ' only. Top-level must be {}. Keys can be dicts, lists, strings
// or ints (which are converted to strings).
Value GypDictLoad(const string& source) {
  Value ret;
  return ret;
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
  Value result = GypDictLoad(build_file_contents);

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
