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

class Buffer {
public:
  Buffer() : _blocks() {}
  Buffer(Buffer&&) = default;
  Buffer& operator=(Buffer&&) = default;
  ~Buffer() = default;
  void append(Block&& block) { _blocks.push_front(std::move(block)); }
  void append(std::size_t size) {
    _blocks.push_front(Block{std::make_unique<std::byte[]>(size), size});
  }
  void clear() { _blocks.clear(); }
  Block& front() { return _blocks.front(); }

  std::forward_list<Block> _blocks;
};
XSL_NET_IO_NE
#endif
