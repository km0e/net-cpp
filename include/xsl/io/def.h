/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
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
#  include <optional>
#  include <span>
XSL_IO_NB

struct Result {
  std::size_t size;
  std::optional<errc> err;

  Result(std::size_t sz, std::optional<errc> e) : size(sz), err(e) {}
  Result(std::size_t sz) : size(sz), err(std::nullopt) {}

  bool operator!() const { return err.has_value(); }
  operator bool() const { return !err.has_value(); }
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
concept AsyncRead = requires(Device t, std::span<byte> buf) {
  { t.read(buf) } -> std::same_as<Task<Result>>;
};

template <class Device>
concept AsyncWrite = requires(Device t, std::span<const byte> buf) {
  { t.write(buf) } -> std::same_as<Task<Result>>;
};

template <class Device>
concept AsyncReadWrite = AsyncRead<Device> && AsyncWrite<Device>;

XSL_IO_NE
#endif  // XSL_IO_DEF
