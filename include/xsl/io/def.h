/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.12
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO_DEF
#  define XSL_IO_DEF
#  define XSL_IO_NB namespace xsl::io {
#  define XSL_IO_NE }
#  include "xsl/byte.h"
#  include "xsl/coro.h"

#  include <concepts>
#  include <cstddef>
#  include <span>
XSL_IO_NB

struct Result {
  std::size_t size;
  errc ec;

  Result() : size(0), ec{} {}
  Result(std::size_t sz, errc e) : size(sz), ec(e) {}
  Result(std::size_t sz) : size(sz), ec{} {}

  bool operator!() const { return ec != errc{}; }
  operator bool() const { return ec == errc{}; }

  decltype(auto) message() const { return std::make_error_code(ec).message(); }
};

template <class Device>
concept Read = requires(Device t, std::span<byte> buf) {
  { t.read(buf) } -> std::same_as<Result>;
};

template <class Device>
concept Write = requires(Device t, std::span<const byte> buf) {
  { t.write(buf) } -> std::same_as<Result>;
};

template <class Device>
concept ReadWrite = Read<Device> && Write<Device>;

template <class Device>
concept AsyncRead = requires(Device t, byte* data, std::size_t size) {
  { t.read(data, size) } -> coro::Awaitable;
  requires std::same_as<typename decltype(t.read(data, size))::result_type, Result>;
};

template <class Device>
concept AsyncWrite = requires(Device t, const byte* data, std::size_t size) {
  { t.write(data, size) } -> coro::Awaitable;
  requires std::same_as<typename decltype(t.write(data, size))::result_type, Result>;
};

template <class Device>
concept AsyncReadWrite = AsyncRead<Device> && AsyncWrite<Device>;

XSL_IO_NE
#endif  // XSL_IO_DEF
