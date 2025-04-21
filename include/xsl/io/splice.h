/**
 * @file splice.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Splice the data from the input device to the output device
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO_SPLICE
#  define XSL_IO_SPLICE
#  include "xsl/coro.h"
#  include "xsl/io/def.h"
#  include "xsl/logctl.h"

XSL_IO_NB

namespace {
  template <AsyncRead R, AsyncWrite W>
  Task<Result> splice_once(R& from, W& to, std::string& buffer) {
    Result res = co_await from.read(xsl::as_writable_bytes(std::span(buffer)));
    if (!res) {
      log_warning("Failed to read data from the device, err: {}",
                  std::make_error_code(res.err.value()).message());
      co_return std::move(res);
    }
    log_debug("Read {} bytes from the device", res.size);
    res = co_await to.write(xsl::as_bytes(std::span(buffer).subspan(0, res.size)));
    if (!res) {
      log_warning("Failed to write data to the device, err: {}",
                  std::make_error_code(res.err.value()).message());
    } else {
      log_debug("Write {} bytes to the device", res.size);
    }
    co_return std::move(res);
  }
}  // namespace

template <AsyncRead R, AsyncWrite W>
constexpr Task<Result> splice_once(R* from, R* to, std::string buffer) {
  return splice_once(*from, *to, buffer);
}

template <AsyncRead R, AsyncWrite W>
Task<Result> splice(R* from, W* to, std::string buffer) {
  std::size_t total = 0;
  while (true) {
    auto [sz, err] = co_await splice_once(*from, *to, buffer);
    if (err) {
      co_return {total, err};
    }
    total += sz;
  }
  log_debug("Spliced {} bytes", total);
}

XSL_IO_NE
#endif
