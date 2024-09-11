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
#include "xsl/def.h"
#ifndef XSL_IO_BYTE
#  define XSL_IO_BYTE
#  include "xsl/feature.h"
#  include "xsl/io/ai.h"
#  include "xsl/io/def.h"
#  include "xsl/io/dyn.h"
#  include "xsl/sys/io.h"
#  include "xsl/type_traits.h"

#  include <concepts>
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
  using dynamic_type = ByteBuffer;
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
  Task<io::Result> write(ABW &awd) {
    std::size_t total_size = 0;
    for (auto &block : _blocks) {
      auto [size, err] = co_await awd.write(block.span());
      if (err) {
        co_return std::make_tuple(total_size, err);
      }
      total_size += size;
    }
    co_return std::make_tuple(total_size, std::nullopt);
  };

  std::forward_list<Block> _blocks;

protected:
  ByteBuffer(std::forward_list<Block> &&blocks) : _blocks(std::move(blocks)) {}
};

template <>
struct AIOTraits<AsyncReadDevice<byte>> {
  using value_type = byte;
  using in_dev_type = AsyncReadDevice<value_type>;
  /// @brief Read from the device
  static constexpr Task<Result> read(in_dev_type &dev, std::span<value_type> buf) {
    return dev.read(buf);
  }
};

template <>
struct AIOTraits<AsyncWriteDevice<byte>> {
  using value_type = byte;
  using out_dev_type = AsyncWriteDevice<value_type>;
  /// @brief Write to the device
  static constexpr Task<Result> write(out_dev_type &dev, std::span<const value_type> buf) {
    return dev.write(buf);
  }
  /// @brief Write to the device
  static Task<Result> write_file(out_dev_type &dev, WriteFileHint &&hint) {
    return _sys::write_file(dev, std::move(hint));
  }
};

template <>
struct AIOTraits<AsyncReadWriteDevice<byte>> {
  using value_type = byte;
  using io_dev_type = AsyncReadWriteDevice<value_type>;
  using in_dev_type = io_dev_type::template rebind<In>;
  using out_dev_type = io_dev_type::template rebind<Out>;
  /// @brief Read from the device
  static constexpr Task<Result> read(io_dev_type &dev, std::span<value_type> buf) {
    return dev.read(buf);
  }
  /// @brief Write to the device
  static constexpr Task<Result> write(io_dev_type &dev, std::span<const value_type> buf) {
    return dev.write(buf);
  }
  /// @brief Write to the device
  static constexpr Task<Result> write_file(io_dev_type &dev, WriteFileHint &&hint) {
    return _sys::write_file(dev, std::move(hint));
  }
};

template <class Device>
concept BRL = ReadDeviceLike<Device, byte>;

template <class Device>
concept BWL = WriteDeviceLike<Device, byte>;

template <class Device>
concept BRWL = ReadWriteDeviceLike<Device, byte>;

template <class Dev>
concept ABIOLike = requires {
  requires std::same_as<byte, typename AIOTraits<Dev>::value_type>;
  typename AIOTraits<Dev>::io_dev_type;
};

template <class Dev>
concept ABILike = requires {
  requires std::same_as<byte, typename AIOTraits<Dev>::value_type>;
  typename AIOTraits<Dev>::in_dev_type;
};

template <class Dev>
concept ABOLike = requires {
  requires std::same_as<byte, typename AIOTraits<Dev>::value_type>;
  typename AIOTraits<Dev>::out_dev_type;
};

XSL_IO_NE
#endif
