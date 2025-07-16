/**
 * @file buf.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_IO_BUF
#  define XSL_IO_BUF
#  include "xsl/io/def.h"

#  include <memory>
XSL_IO_NB

template <std::size_t BlockSize>
class Buffer {
public:
  Buffer() { _buffers.emplace_back(std::make_unique<byte[]>(BlockSize)); }
  Buffer(const Buffer&) = delete;
  Buffer(Buffer&&) = default;
  Buffer& operator=(const Buffer&) = delete;
  Buffer& operator=(Buffer&&) = default;

  std::tuple<std::size_t, errc> write(const byte* data, std::size_t size) {
    if (_current_size + size > BlockSize) {
      std::copy_n(data, BlockSize - _current_size, _buffers.back().get() + _current_size);
      data += (BlockSize - _current_size);
      size -= (BlockSize - _current_size);
      _buffers.emplace_back(std::make_unique<byte[]>(BlockSize));
    }
    while (size >= BlockSize) {
      std::copy_n(data, BlockSize, _buffers.back().get());
      _buffers.emplace_back(std::make_unique<byte[]>(BlockSize));
      data += BlockSize;
      size -= BlockSize;
    }
    std::copy_n(data, size, _buffers.back().get() + _current_size);
    _current_size += size;
    return {_current_size, errc{}};
  }

  std::size_t current_size() const { return _current_size; }
  std::size_t& current_size() { return _current_size; }

  std::vector<std::unique_ptr<byte[]>>& buffers() { return _buffers; }

  /**
   * @brief Add a new buffer to the end of the buffer list.
   *
   * @param buffer The buffer to add, must be a unique pointer to a byte array, and the size must
   * greater than BlockSize.
   */
  void add_buffer(std::unique_ptr<byte[]>&& buffer) { _buffers.push_back(std::move(buffer)); }

  /**
   * @brief Check if the last buffer is full.
   *
   * @return true if the last buffer is full, false otherwise
   */
  bool is_full() const { return _current_size == BlockSize; }

protected:
  std::vector<std::unique_ptr<byte[]>> _buffers = {};  ///< Buffers to hold the data
  std::size_t _current_size = 0;                       ///< Current size of the buffer
};

XSL_IO_NE
#endif  // XSL_IO_BUF
