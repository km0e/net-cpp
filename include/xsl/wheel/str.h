#pragma once
#include <compare>
#ifndef _XSL_WHEEL_STR_H_
#  define _XSL_WHEEL_STR_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/giant.h"

WHEEL_NAMESPACE_BEGIN
using std::to_string;
template <typename T>
concept ToString = requires(T t) {
  { to_string(t) } -> giant::convertible_to<giant::string_view>;
} || requires(T t) {
  { t.to_string() } -> giant::convertible_to<giant::string_view>;
};

template <typename T>
concept Stringable = requires(T t) {
  { t.to_string() } -> giant::convertible_to<giant::string_view>;
};

template <Stringable T>
giant::string to_string(T t) {
  return t.to_string();
}
template <typename T>
T from_string(giant::string_view str);

class FixedString {
public:
  FixedString(size_t size) : _size(size), _data(new char[size]) {}
  FixedString(const char* str, size_t size) : _size(size), _data(new char[size]) {
    std::copy(str, str + size, this->_data.get());
  }
  FixedString(giant::string_view str) : _size(str.size()), _data(new char[str.size()]) {
    std::copy(str.begin(), str.end(), this->_data.get());
  }
  FixedString(const FixedString& other) : _size(other._size), _data() {
    if (this == &other) {
      return;
    }
    if (other._data) {
      this->_data = giant::make_unique<char[]>(this->_size);
      std::copy(other._data.get(), other._data.get() + this->_size, this->_data.get());
    }
  }
  FixedString(FixedString&& other) : _size(other._size), _data() {
    if (this == &other) {
      return;
    }
    this->_data = std::move(other._data);
  }
  FixedString& operator=(const FixedString& other) {
    if (this == &other) {
      return *this;
    }
    if (this->_size != other._size) {
      this->_size = other._size;
      this->_data = giant::make_unique<char[]>(this->_size);
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
      this->_data = giant::make_unique<char[]>(this->_size);
    }
    std::copy(str, str + this->_size, this->_data.get());
    return *this;
  }
  char& at(size_t index) { return this->_data[index]; }
  const char& at(size_t index) const { return this->_data[index]; }
  char& operator[](size_t index) { return this->at(index); }
  const char& operator[](size_t index) const { return this->at(index); }
  size_t size() const { return this->_size; }
  int compare(const giant::string_view& other) const {
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
    return this->compare(giant::string_view(other.data(), other.size()));
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

private:
  size_t _size;
  giant::unique_ptr<char[]> _data;
};

// @brief compare two FixedString
// @param lhs the left hand side FixedString
// @param rhs the right hand side FixedString
// @return true if lhs is equal to rhs, otherwise false

inline std::strong_ordering operator<=>(const FixedString& lhs, const FixedString& rhs) {
  return lhs.compare(giant::string_view(rhs.data(), rhs.size())) <=> 0;
}
inline std::strong_ordering operator<=>(const FixedString& lhs, giant::string_view rhs) {
  return lhs.compare(rhs) <=> 0;
}
inline std::strong_ordering operator<=>(const FixedString& lhs, const char* rhs) {
  return lhs.compare(giant::string_view(rhs)) <=> 0;
}

WHEEL_NAMESPACE_END
#endif
