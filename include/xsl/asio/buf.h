/**
 * @file buf.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_ASIO_BUF
#  define XSL_ASIO_BUF
#  include <xsl/asio/def.h>
#  include <xsl/io.h>

#  include <memory>
XSL_ASIO_NB

template <std::size_t BlockSize>
class Buffer : public io::Buffer<BlockSize> {
public:
  using io::Buffer<BlockSize>::Buffer, io::Buffer<BlockSize>::write;

  template <AsyncRead R>
  Task<io::Result> write(R& reader) {
    log_debug("Buffer::write called, buffer size: {}, current size: {}", this->_buffers.size(),
              this->_current_size);
    if (this->_current_size == BlockSize) {
      this->_buffers.emplace_back(std::make_unique<byte[]>(BlockSize));
      this->_current_size = 0;
    }
    auto res = co_await reader->read(this->_buffers.back().get() + this->_current_size,
                                     BlockSize - this->_current_size);
    if (!res) {
      co_return res;
    }
    this->_current_size += res.size;
    co_return res;
  }
};

XSL_ASIO_NE
#endif  // XSL_IO_BUF
