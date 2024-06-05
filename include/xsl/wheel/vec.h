#pragma once
#ifndef _XSL_WHEEL_VEC_H_
#  define _XSL_WHEEL_VEC_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/giant.h"
WHEEL_NAMESPACE_BEGIN
template <typename T>
class FixedVec {
public:
  FixedVec() : _size(0), data() {}
  FixedVec(size_t size) : _size(size), data(new T[size]) {}
  FixedVec(const T* arr, size_t size) : _size(size), data(new T[size]) {
    std::copy(arr, arr + size, this->data.get());
  }
  FixedVec(const FixedVec& other) : _size(other._size), data() {
    if (other.data) {
      this->data = giant::make_unique<T[]>(this->_size);
      std::copy(other.data.get(), other.data.get() + this->_size, this->data.get());
    }
  }
  FixedVec(FixedVec&& other) : _size(other._size), data() {
    this->data = std::move(other.data);
  }
  FixedVec& operator=(const FixedVec& other) {
    if (this == &other) {
      return *this;
    }
    if (this->_size != other._size) {
      this->_size = other._size;
      this->data = giant::make_unique<T[]>(this->_size);
    }
    if (other.data) {
      std::copy(other.data.get(), other.data.get() + this->_size, this->data.get());
    }
    return *this;
  }
  FixedVec& operator=(FixedVec&& other) {
    if (this == &other) {
      return *this;
    }
    this->_size = other._size;
    this->data = std::move(other.data);
    return *this;
  }
  T& operator[](size_t index) { return this->data[index]; }
  const T& operator[](size_t index) const { return this->data[index]; }
  size_t size() const { return this->_size; }

  T* begin() { return this->data.get(); }
  const T* begin() const { return this->data.get(); }
  T* end() { return this->data.get() + this->_size; }
  const T* end() const { return this->data.get() + this->_size; }

private:
  size_t _size;
  giant::unique_ptr<T[]> data;
};
WHEEL_NAMESPACE_END
#endif
