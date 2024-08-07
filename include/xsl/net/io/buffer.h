/**
@file buffer.h
@author Haixin Pang (kmdr.error@gmail.com)
@brief
@version 0.1
@date 2024-08-04

@copyright Copyright (c) 2024

 */
#pragma once
#ifndef XSL_NET_IO_BUFFER
#  define XSL_NET_IO_BUFFER
#  include "xsl/ai/dev.h"
#  include "xsl/feature.h"
#  include "xsl/net/io/def.h"

#  include <cstddef>
#  include <forward_list>
#  include <memory>
#  include <span>
XSL_NET_IO_NB
class Block {
public:
  /**
  @brief Construct a new Block object

  @param size the size of the block
   */
  Block(std::size_t size) : data(std::make_unique<std::byte[]>(size)), valid_size(size) {}
  /**
  @brief Construct a new Block object

  @param data the data
  @param size the valid size of the data
   */
  Block(std::unique_ptr<std::byte[]> data, std::size_t size)
      : data(std::move(data)), valid_size(size) {}
  Block(Block&&) = default;
  Block& operator=(Block&&) = default;
  ~Block() = default;

  std::span<std::byte> span() { return {data.get(), valid_size}; }
  std::span<std::byte> span(std::size_t offset) {
    return {data.get() + offset, valid_size - offset};
  }

  std::unique_ptr<std::byte[]> data;  ///< the data
  std::size_t valid_size;             ///< the valid size of the data
};

namespace impl_buffer {
  template <class... Flags>
  class Buffer;

  template <class... Flags>
  using BufferCompose
      = feature::origanize_feature_flags_t<impl_buffer::Buffer<feature::Dyn>, Flags...>;

  template <class T>
  class Buffer<T> : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                              ai::AsyncWritable<std::byte>, feature::placeholder> {
  public:
    using value_type = std::byte;

    Buffer() : _blocks() {}
    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;
    ~Buffer() = default;
    void append(Block&& block) { _blocks.push_front(std::move(block)); }
    void append(std::size_t size) {
      _blocks.push_front(Block{std::make_unique<value_type[]>(size), size});
    }
    void clear() { _blocks.clear(); }
    Block& front() { return _blocks.front(); }

    coro::Task<ai::Result> write(ai::AsyncDevice<feature::Out<value_type>>& awd) {
      std::size_t total_size = 0;
      for (auto& block : _blocks) {
        auto [size, err] = co_await awd.write(block.span());
        if (err) {
          co_return std::make_tuple(total_size, err);
        }
        total_size += size;
      }
      co_return std::make_tuple(total_size, std::nullopt);
    };

    BufferCompose<feature::Dyn> to_dyn() { return BufferCompose<feature::Dyn>(std::move(_blocks)); }

    std::forward_list<Block> _blocks;

  protected:
    Buffer(std::forward_list<Block>&& blocks) : _blocks(std::move(blocks)) {}
  };
}  // namespace impl_buffer

template <class... Flags>
using Buffer = impl_buffer::BufferCompose<Flags...>;

XSL_NET_IO_NE
#endif
