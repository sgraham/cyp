// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "input/input.h"

#include <map>
#include <set>
#include <string>
#include <vector>

using std::map;
using std::set;
using std::string;
using std::vector;

typedef map<string, set<string> > LoadData;
typedef map<string, string> StringDict;

void ReadFile(const string& path, char** data, size_t* len) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f)
    Fatal("couldn't open '%s': %s", path.c_str(), strerror(errno));
  fseek(f, 0, SEEK_END);
  *len = ftell(f);
  fseek(f, 0, SEEK_SET);
  *data = new char[*len];
  size_t read = fread(data, 1, *len, f);
  if (read != *len)
    Fatal("didn't read %d bytes as expected", *len);
}

void LoadOneBuildFile(const string& build_file_path,
                      const LoadData& data,
                      void* aux_data,  // TODO: ?
                      const StringDict& variables,
                      const vector<string>& includes,
                      bool is_target) {
  // TODO: If already loaded, return already loaded result.

  // Read file, or abort.
  char* build_file_contents;
  size_t build_file_len;
  ReadFile(build_file_path, &build_file_contents, &build_file_len);

  // "eval" the contents of the file, reporting syntax and evaluation errors.
  Value result;
  std::string err;
  GypLoad(build_file_path, build_file_contents, build_file_len, &result, &err);

  // Ensure it evalutes to a dictionary.
  if (result.IsDict())
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

