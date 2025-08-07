/**
 * @file buf.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Some buffer classes for I/O operations
 * @version 0.1.0
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_IO_BUF
#  define XSL_IO_BUF
#  include <xsl/io/def.h>

#  include <memory>
XSL_IO_NB

template <std::size_t BlockSize>
class FixedBuffer {
public:
  FixedBuffer() : _data(std::make_unique<byte[]>(BlockSize)), _size(0) {}
  FixedBuffer(std::unique_ptr<byte[]>&& data, std::size_t size = 0)
      : _data(std::move(data)), _size(size) {
    assert(_data && "Data pointer cannot be null");
    assert(_size <= BlockSize && "Size exceeds block size");
  }
  FixedBuffer(const FixedBuffer&) = delete;
  FixedBuffer(FixedBuffer&&) = default;
  FixedBuffer& operator=(const FixedBuffer&) = delete;
  FixedBuffer& operator=(FixedBuffer&&) = default;

  /**
   * @brief Get the data pointer of the buffer.
   *
   * @param self The FixedBuffer instance.
   * @return A pointer to the data of the buffer.
   */
  constexpr auto data(this auto&& self) { return self._data.get(); }
  /**
   * @brief Get the internal data of the buffer.
   *
   * @param self The FixedBuffer instance.
   * @return A reference to the internal data of the buffer.
   */
  constexpr auto&& underlying(this auto&& self) { return std::forward<decltype(self)>(self)._data; }
  /**
   * @brief Get the data pointer of the unfilled part of the buffer.
   *
   * @return A pointer to the unfilled part of the buffer.
   */
  constexpr byte* unfilled() { return _data.get() + _size; }
  /**
   * @brief update the valid size of the buffer.
   *
   * @param size The size just filled in the buffer.
   */
  constexpr void fill(std::size_t size) {
    assert(size <= BlockSize - _size && "Buffer overflow");
    _size += size;
  }
  /**
   * @brief Set the size of the valid data in the buffer.
   *
   * @param size The new size of the valid data in the buffer.
   */
  constexpr void resize(std::size_t size = 0) {
    assert(size <= BlockSize && "Buffer size exceeds block size");
    _size = size;
  }
  /**
   * @brief Get the current valid size of the buffer.
   *
   * @return The current valid size of the buffer.
   */
  constexpr std::size_t size() const { return _size; }

private:
  std::unique_ptr<byte[]> _data;  ///< Pointer to the buffer data
  std::size_t _size = 0;          ///< Current size of the buffer
};

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
