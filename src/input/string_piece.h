// // Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INPUT_STRING_PIECE_H_
#define INPUT_STRING_PIECE_H_

#include <string>

using namespace std;

#include <string.h>

struct StringPiece {
  StringPiece() : str_(NULL), len_(0) {}

  // Intentionally allow for implicit conversions.
  StringPiece(const string& str) : str_(str.data()), len_(str.size()) {}
  StringPiece(const char* str) : str_(str), len_(strlen(str)) {}

  StringPiece(const char* str, size_t len) : str_(str), len_(len) {}

  bool operator==(const StringPiece& other) const {
    return len_ == other.len_ && memcmp(str_, other.str_, len_) == 0;
  }
  bool operator!=(const StringPiece& other) const {
    return !(*this == other);
  }

  // Convert the slice into a full-fledged std::string, copying the data into a
  // new string.
  string AsString() const {
    return len_ ? string(str_, len_) : string();
  }

  const char* str_;
  size_t len_;
};

#endif  // INPUT_STRING_PIECE_H_
