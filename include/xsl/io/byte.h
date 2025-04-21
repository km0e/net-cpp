/**
 * @file byte.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Byte IO utilities
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO_BYTE
#  define XSL_IO_BYTE
#  include "xsl/io/ai.h"
#  include "xsl/io/def.h"
#  include "xsl/io/dyn.h"
#  include "xsl/type_traits.h"

#  include <forward_list>
XSL_IO_NB

class Block {
public:
  /// @brief Construct a new Block object
  constexpr Block(std::size_t size) : data(std::make_unique<byte[]>(size)), valid_size(size) {}
  /// @brief Construct a new Block object
  constexpr Block(std::unique_ptr<byte[]> data, std::size_t size)
      : data(std::move(data)), valid_size(size) {}
  constexpr Block(Block &&) = default;
  constexpr Block &operator=(Block &&) = default;
  constexpr ~Block() = default;
  /// @brief Get the span of the data
  constexpr std::span<byte> span(std::size_t offset = 0) {
    return {data.get() + offset, valid_size - offset};
  }
  constexpr std::span<const byte> span(std::size_t offset = 0) const {
    return {data.get() + offset, valid_size - offset};
  }
  constexpr byte *invalid_begin() { return data.get() + valid_size; }
  constexpr std::span<byte> invalid_span(std::size_t size) {
    return {data.get() + valid_size, size};
  }

  std::unique_ptr<byte[]> data;  ///< the data
  std::size_t valid_size;        ///< the valid size of the data
};

class ByteBuffer {
public:
  using value_type = byte;
  using io_dyn_chains = _n<ByteBuffer, io::DynAsyncWriteBuffer<ByteBuffer>>;

  ByteBuffer() : _blocks() {}
  ByteBuffer(ByteBuffer &&) = default;
  ByteBuffer &operator=(ByteBuffer &&) = default;
  ~ByteBuffer() = default;
  constexpr void append(Block &&block) { _blocks.push_front(std::move(block)); }
  constexpr void append(std::size_t size) {
    _blocks.push_front(Block{std::make_unique<value_type[]>(size), size});
  }
  constexpr void clear() { _blocks.clear(); }
  constexpr Block &front() { return _blocks.front(); }
  /// @brief Write the buffer to the given AsyncWriteDevice
  Task<io::Result> write(AsyncWriteDevice &awd) {
    std::size_t total_size = 0;
    for (auto &block : _blocks) {
      auto [size, err] = co_await awd.write(block.span());
      if (err) {
        co_return {total_size, err};
      }
      total_size += size;
    }
    co_return total_size;
  };

  std::forward_list<Block> _blocks;

protected:
  ByteBuffer(std::forward_list<Block> &&blocks) : _blocks(std::move(blocks)) {}
};

XSL_IO_NE
#endif
