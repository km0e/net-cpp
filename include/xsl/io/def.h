/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.2
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

#  include <xsl/byte.h>

#  include <concepts>
#  include <cstddef>
#  include <cstring>
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

  decltype(auto) message() const { return std::strerror(static_cast<int>(ec)); }
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

XSL_IO_NE
#endif  // XSL_IO_DEF
