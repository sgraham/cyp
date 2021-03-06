// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INPUT_INPUT_H_
#define INPUT_INPUT_H_

#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN __attribute__((noreturn))
#endif

#include <assert.h>

#include <string>
#include <unordered_map>
#include <vector>

NORETURN void Fatal(const char* msg, ...);

// VS2013 doesn't yet support unrestricted unions.
class Value {
 public:
  Value() : flags_(0) {}

  bool IsDict() const { return flags_ & kDict; }
  bool IsList() const { return flags_ & kList; }
  bool IsString() const { return flags_ & kString; }

  Value& SetNone() {
    this->~Value();
    new (this) Value();
    return *this;
  }

  Value& SetList() {
    this->~Value();
    new (this) Value(kList);
    return *this;
  }

  size_t GetListSize() const {
    assert(IsList());
    return data_.l->size();
  }

  size_t GetListCapacity() const {
    assert(IsList());
    return data_.l->capacity();
  }

  void ListClear() {
    assert(IsList());
    data_.l->clear();
  }

  Value& operator[](size_t index) {
    assert(IsList());
    return data_.l->at(index);
  }

  void ListPushBack(const Value& value) {
    assert(IsList());
    data_.l->push_back(value);
  }

  void ListPopBack() {
    assert(IsList());
    data_.l->pop_back();
  }

  void ListReserve(size_t size) {
    assert(IsList());
    data_.l->reserve(size);
  }

  // TODO: Make source data mutable to handle escapes, and then make this a
  // StringPiece.
  Value& SetString(const std::string& str) {
    this->~Value();
    new (this) Value(str);
    return *this;
  }

  const std::string& GetString() const {
    assert(IsString());
    return *data_.s;
  }

  size_t GetStringLength() const {
    assert(IsString());
    return data_.s->size();
  }

  Value& SetDict() {
    this->~Value();
    new (this) Value(kDict);
    return *this;
  }

  void SetItem(const std::string& key, const Value& value) {
    assert(IsDict());
    // TODO: This should replace probably?
    data_.d->emplace(key, value);
  }

  size_t GetDictSize() const {
    assert(IsDict());
    return data_.d->size();
  }

  const Value& GetItem(const std::string& key) const {
    assert(IsDict());
    return data_.d->at(key);
  }

  Value& operator=(Value& rhs) {
    Value temp(rhs);
    std::swap(this->data_.ptr, rhs.data_.ptr);
    std::swap(this->flags_, rhs.flags_);
    return *this;
  }

  Value(const Value& rhs) : flags_(rhs.flags_) {
    if (flags_ & kDict) {
      data_.d = new std::unordered_map<std::string, Value>(rhs.data_.d->begin(),
                                                           rhs.data_.d->end());
    } else if (flags_ & kList) {
      data_.l =
          new std::vector<Value>(rhs.data_.l->begin(), rhs.data_.l->end());
    } else if (flags_ & kString) {
      data_.s = new std::string(*rhs.data_.s);
    }
  }

 private:
  enum ValueFlags {
    kDict = 0x01,
    kList = 0x02,
    kString = 0x04,
  };

  Value(ValueFlags flags) : flags_(flags) {
    if (flags & kDict)
      data_.d = new std::unordered_map<std::string, Value>();
    else if (flags & kList)
      data_.l = new std::vector<Value>();
    else if (flags & kString)
      data_.s = new std::string();
  }

  Value(const std::string& str) : flags_(kString) {
    data_.s = new std::string(str);
  }

  union Data {
    std::unordered_map<std::string, Value>* d;
    std::vector<Value>* l;
    std::string* s;
    void* ptr;
  };

  Data data_;
  unsigned int flags_;
};

void GypLoad(const std::string& data, Value* result, std::string* err);

#endif  // INPUT_INPUT_H_
