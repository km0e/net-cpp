/**
 * @file str.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_STR
#  define XSL_STR
#  include "xsl/def.h"

#  include <algorithm>
#  include <compare>
#  include <cstddef>
#  include <cstring>
#  include <memory>
#  include <string>
#  include <string_view>

XSL_NB

// template <typename T>
// constexpr T from_string_view(std::string_view str);

// for std::error_code
constexpr std::string to_string(const std::error_code& ec) { return ec.message(); }

// for errc
inline std::string to_string(const errc& ec) { return std::make_error_code(ec).message(); }

template <typename T>
concept ToString = std::convertible_to<T, std::string> || requires(T t) {
  { to_string(t) } -> std::convertible_to<std::string>;
} || requires(T t) {
  { t.to_string() } -> std::convertible_to<std::string>;
};

template <class T>
  requires requires(T t) {
    { t.to_string() } -> std::convertible_to<std::string>;
  }
std::string to_string(T&& t) {
  return t.to_string();
}

template <class T>
  requires std::convertible_to<T, std::string>
std::string to_string(T&& t) {
  return std::forward<T>(t);
}

template <class T>
concept ToStringView = std::convertible_to<T, std::string_view> || requires(T t) {
  { to_string_view(t) } -> std::convertible_to<std::string_view>;
} || requires(T t) {
  { t.to_string_view() } -> std::convertible_to<std::string_view>;
};

template <class T>
  requires requires(T t) {
    { t.to_string_view() } -> std::convertible_to<std::string_view>;
  }
std::string_view to_string_view(T&& t) {
  return t.to_string_view();
}

template <class T>
  requires std::convertible_to<T, std::string_view>
std::string_view to_string_view(T&& t) {
  return std::forward<T>(t);
}

class FixedString {
public:
  constexpr FixedString() : _size(0), _data() {}
  constexpr FixedString(size_t size) : _size(size), _data(new char[size]) {}
  constexpr FixedString(const char* str, size_t size) : _size(size), _data(new char[size]) {
    if (str == nullptr || size == 0) {
      return;
    }
    std::copy(str, str + size, this->_data.get());
  }
  FixedString(const char* str) : FixedString(str, std::strlen(str)) {}
  constexpr FixedString(std::string_view str) : FixedString(str.data(), str.size()) {}
  constexpr FixedString(const FixedString& other) : FixedString(other.data(), other.size()) {}
  constexpr FixedString(FixedString&& other) : _size(other._size), _data() {
    this->_data = std::move(other._data);
  }
  constexpr FixedString(std::string&& str) : FixedString(str.data(), str.size()) {}
  constexpr FixedString(const std::string& str) : FixedString(str.data(), str.size()) {}
  constexpr FixedString& operator=(const FixedString& other) {
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
  constexpr FixedString& operator=(FixedString&& other) {
    if (this == &other) {
      return *this;
    }
    this->_size = other._size;
    this->_data = std::move(other._data);
    return *this;
  }
  constexpr ~FixedString() = default;
  constexpr FixedString& assign(const char* str) {
    if (this->_size == 0 || str == nullptr) {
      return *this;
    }
    if (!this->_data) {
      this->_data = std::make_unique<char[]>(this->_size);
    }
    std::copy(str, str + this->_size, this->_data.get());
    return *this;
  }
  constexpr char& at(size_t index) { return this->_data[index]; }
  constexpr const char& at(size_t index) const { return this->_data[index]; }
  constexpr char& operator[](size_t index) { return this->at(index); }
  constexpr const char& operator[](size_t index) const { return this->at(index); }
  constexpr size_t size() const { return this->_size; }
  constexpr int compare(const std::string_view& other) const {
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
  constexpr int compare(const FixedString& other) const {
    return this->compare(std::string_view(other.data(), other.size()));
  }
  constexpr char* data() { return this->_data.get(); }
  constexpr const char* data() const { return this->_data.get(); }
  constexpr bool operator==(const char* other) const {
    size_t i = 0;
    for (; (i < this->size()) && other[i]; ++i) {
      if (this->at(i) != other[i]) {
        return false;
      }
    }
    return i == this->size() && other[i] == '\0';
  }
  constexpr std::string to_string() const { return std::string(this->data(), this->size()); }
  constexpr std::string_view to_string_view() const {
    return std::string_view(this->data(), this->size());
  }

private:
  size_t _size;
  std::unique_ptr<char[]> _data;
};

constexpr std::string_view to_string_view(const FixedString& str) {
  return std::string_view(str.data(), str.size());
}

// @brief compare two FixedString
// @param lhs the left hand side FixedString
// @param rhs the right hand side FixedString
// @return true if lhs is equal to rhs, otherwise false

constexpr std::strong_ordering operator<=>(const FixedString& lhs, const FixedString& rhs);
constexpr std::strong_ordering operator<=>(const FixedString& lhs, std::string_view rhs);
constexpr std::strong_ordering operator<=>(const FixedString& lhs, const char* rhs);
XSL_NE
#endif
