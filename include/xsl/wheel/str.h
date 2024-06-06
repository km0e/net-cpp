#pragma once
#ifndef _XSL_WHEEL_STR_H_
#  define _XSL_WHEEL_STR_H_
#  include "xsl/wheel/def.h"

#  include <compare>
#  include <cstring>
#  include <memory>
#  include <string>
#  include <string_view>

WHEEL_NAMESPACE_BEGIN

void i32_to_bytes(int32_t value, char* bytes);

int32_t i32_from_bytes(const char* bytes);

void bool_to_bytes(bool value, char* bytes);

bool bool_from_bytes(const char* bytes);

class FixedString {
public:
  FixedString() : _size(0), _data() {}
  FixedString(size_t size) : _size(size), _data(new char[size]) {}
  FixedString(const char* str, size_t size) : _size(size), _data(new char[size]) {
    if (str == nullptr || size == 0) {
      return;
    }
    std::copy(str, str + size, this->_data.get());
  }
  FixedString(const char* str) : FixedString(str, std::strlen(str)) {}
  FixedString(std::string_view str) : FixedString(str.data(), str.size()) {}
  FixedString(const FixedString& other) : FixedString(other.data(), other.size()) {}
  FixedString(FixedString&& other) : _size(other._size), _data() {
    this->_data = std::move(other._data);
  }
  FixedString(std::string&& str) : FixedString(str.data(), str.size()) {}
  FixedString(const std::string& str) : FixedString(str.data(), str.size()) {}
  FixedString& operator=(const FixedString& other) {
    if (this == &other) {
      return *this;
    }
    if (this->_size != other._size) {
      this->_size = other._size;
      this->_data = std::make_unique<char[]>(this->_size);
    }
    if (other._data) {
      std::copy(other._data.get(), other._data.get() + this->_size, this->_data.get());
    }
    return *this;
  }
  FixedString& operator=(FixedString&& other) {
    if (this == &other) {
      return *this;
    }
    this->_size = other._size;
    this->_data = std::move(other._data);
    return *this;
  }
  ~FixedString() = default;
  FixedString& assign(const char* str) {
    if (this->_size == 0 || str == nullptr) {
      return *this;
    }
    if (!this->_data) {
      this->_data = std::make_unique<char[]>(this->_size);
    }
    std::copy(str, str + this->_size, this->_data.get());
    return *this;
  }
  char& at(size_t index) { return this->_data[index]; }
  const char& at(size_t index) const { return this->_data[index]; }
  char& operator[](size_t index) { return this->at(index); }
  const char& operator[](size_t index) const { return this->at(index); }
  size_t size() const { return this->_size; }
  int compare(const std::string_view& other) const {
    auto min_size = std::min(this->size(), other.size());
    for (size_t i = 0; i < min_size; ++i) {
      if (this->at(i) < other[i]) {
        return -1;
      } else if (this->at(i) > other[i]) {
        return 1;
      }
    }
    return this->size() > other.size() ? 1 : (this->size() < other.size() ? -1 : 0);
  }
  int compare(const FixedString& other) const {
    return this->compare(std::string_view(other.data(), other.size()));
  }
  char* data() { return this->_data.get(); }
  const char* data() const { return this->_data.get(); }
  bool operator==(const char* other) const {
    size_t i = 0;
    for (; (i < this->size()) && other[i]; ++i) {
      if (this->at(i) != other[i]) {
        return false;
      }
    }
    return i == this->size() && other[i] == '\0';
  }
  std::string to_string() const { return std::string(this->data(), this->size()); }
  std::string_view to_string_view() const { return std::string_view(this->data(), this->size()); }

private:
  size_t _size;
  std::unique_ptr<char[]> _data;
};

inline std::string_view to_string_view(const FixedString& str) {
  return std::string_view(str.data(), str.size());
}

// @brief compare two FixedString
// @param lhs the left hand side FixedString
// @param rhs the right hand side FixedString
// @return true if lhs is equal to rhs, otherwise false

std::strong_ordering operator<=>(const FixedString& lhs, const FixedString& rhs);
std::strong_ordering operator<=>(const FixedString& lhs, std::string_view rhs);
std::strong_ordering operator<=>(const FixedString& lhs, const char* rhs);
WHEEL_NAMESPACE_END
#endif
