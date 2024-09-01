/**
 * @file vec.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_WHEEL_VEC
#  define XSL_WHEEL_VEC
#  include "xsl/wheel/def.h"

#  include <algorithm>
#  include <cassert>
#  include <cstddef>
#  include <initializer_list>
#  include <memory>
#  include <vector>
XSL_WHEEL_NB
template <typename T>
class FixedVector {
public:
  constexpr FixedVector() : _data(nullptr), _size(0) {}
  constexpr FixedVector(FixedVector&&) = default;
  constexpr FixedVector(const FixedVector& other) : _data(new T[other._size]), _size(other._size) {
    std::copy(other._data.get(), other._data.get() + other._size, _data.get());
  }
  constexpr FixedVector& operator=(FixedVector&&) = default;
  constexpr FixedVector& operator=(const FixedVector&) {
    FixedVector tmp(*this);
    *this = std::move(tmp);
    return *this;
  }
  /**
   * @brief Construct a new Fixed Vector object
   *
   * @param il initializer list
   */
  constexpr FixedVector(std::initializer_list<T> il) : _data(new T[il.size()]), _size(il.size()) {
    std::size_t i = 0;
    for (const auto& x : il) {
      _data[i++] = x;
    }
  }
  /**
   * @brief Construct a new Fixed Vector object
   *
   * @param size the size of the vector
   * @note the vector is not initialized
   */
  constexpr FixedVector(std::size_t size) : _data(new T[size]), _size(size) {}
  /**
   * @brief Construct a new Fixed Vector object
   *
   * @param size the size of the vector
   * @param value the value to initialize the vector
   */
  constexpr FixedVector(std::size_t size, const T& value) : _data(new T[size]), _size(size) {
    for (std::size_t i = 0; i < size; ++i) {
      _data[i] = value;
    }
  }
  /**
   * @brief Construct a new Fixed Vector object
   *
   * @param vec the vector to copy
   */
  constexpr FixedVector(const std::vector<T>& vec) : _data(new T[vec.size()]), _size(vec.size()) {
    std::copy(vec.begin(), vec.end(), _data.get());
  }
  /**
   * @brief Construct a new Fixed Vector object
   *
   * @param self
   * @param i
   * @return auto&&
   */
  constexpr auto&& at(this auto&& self, std::size_t i) {
    assert(i < self._size);
    return self._data[i];
  }
  /**
   * @brief Construct a new Fixed Vector object
   *
   * @param self
   * @param i
   * @return auto&&
   */
  constexpr auto&& operator[](this auto&& self, std::size_t i) { return self.at(i); }

  constexpr auto begin(this auto&& self) { return self._data.get(); }
  constexpr auto end(this auto&& self) { return self._data.get() + self._size; }

  constexpr bool empty(this auto&& self) { return self.size() == 0; }
  constexpr std::size_t size(this auto&& self) { return self._size; }

private:
  std::unique_ptr<T[]> _data;
  std::size_t _size;
};
XSL_WHEEL_NE
#endif
