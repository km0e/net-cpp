/**
 * @file buffer.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Buffer for IO
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_IO_BUFFER
#  define XSL_NET_IO_BUFFER
#  include "xsl/ai.h"
#  include "xsl/feature.h"
#  include "xsl/net/io/def.h"

#  include <cstddef>
#  include <forward_list>
#  include <memory>
#  include <span>
#  include <type_traits>
XSL_NET_IO_NB
class Block {
public:
  /**
  @brief Construct a new Block object

  @param size the size of the block
   */
  Block(std::size_t size) : data(std::make_unique<byte[]>(size)), valid_size(size) {}
  /**
  @brief Construct a new Block object

  @param data the data
  @param size the valid size of the data
   */
  Block(std::unique_ptr<byte[]> data, std::size_t size) : data(std::move(data)), valid_size(size) {}
  Block(Block&&) = default;
  Block& operator=(Block&&) = default;
  ~Block() = default;

  std::span<byte> span() { return {data.get(), valid_size}; }
  std::span<byte> span(std::size_t offset) { return {data.get() + offset, valid_size - offset}; }

  std::unique_ptr<byte[]> data;  ///< the data
  std::size_t valid_size;        ///< the valid size of the data
};

namespace impl_buffer {
  template <class... Flags>
  class Buffer;

  template <class... Flags>
  using BufferCompose = organize_feature_flags_t<Buffer<Dyn>, Flags...>;

  template <class T>
  class Buffer<T>
      : public std::conditional_t<std::is_same_v<T, Dyn>, ai::AsyncWritable<byte>, Placeholder> {
  public:
    using value_type = byte;

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

    Task<Result> write(ABW& awd) {
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

    BufferCompose<Dyn> to_dyn() { return BufferCompose<Dyn>(std::move(_blocks)); }

    std::forward_list<Block> _blocks;

  protected:
    Buffer(std::forward_list<Block>&& blocks) : _blocks(std::move(blocks)) {}
  };
}  // namespace impl_buffer

template <class... Flags>
using Buffer = impl_buffer::BufferCompose<Flags...>;

XSL_NET_IO_NE
#endif
