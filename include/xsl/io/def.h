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
#  include "xsl/coro.h"

#  include <concepts>
#  include <cstddef>
#  include <optional>
#  include <span>
XSL_IO_NB

template <class Dev>
struct IOTraits;

template <class Dev>
struct AIOTraits;

struct Result {
  std::size_t size;
  std::optional<errc> err;

  Result(std::size_t sz, std::optional<errc> e) : size(sz), err(e) {}
  Result(std::size_t sz) : size(sz), err(std::nullopt) {}

  bool operator!() const { return err.has_value(); }
  operator bool() const { return !err.has_value(); }
};

template <class Device, class T>
concept ReadDeviceLike = requires(Device t, std::span<T> buf) {
  { IOTraits<Device>::read(t, buf) } -> std::same_as<Result>;
};

template <class Device, class T>
concept WriteDeviceLike = requires(Device t, std::span<const T> buf) {
  { IOTraits<Device>::write(t, buf) } -> std::same_as<Result>;
};

template <class Device, class T>
concept ReadWriteDeviceLike = ReadDeviceLike<Device, T> && WriteDeviceLike<Device, T>;

template <class Device, class T>
concept AsyncReadDeviceLike = requires(Device t, std::span<T> buf) {
  { AIOTraits<Device>::read(t, buf) } -> std::same_as<Task<Result>>;
};

template <class Device, class T>
concept AsyncWriteDeviceLike = requires(Device t, std::span<const T> buf) {
  { AIOTraits<Device>::write(t, buf) } -> std::same_as<Task<Result>>;
};

template <class Device, class T>
concept AsyncReadWriteDeviceLike
    = AsyncReadDeviceLike<Device, T> && AsyncWriteDeviceLike<Device, T>;

/// @brief Write file hint
struct WriteFileHint {
  std::string
      path;  ///< file path, must be string, not string_view. Because this function will be called
             ///< in coroutine, and the path may be destroyed before the function is called.
  std::size_t offset;
  std::size_t size;
};

XSL_IO_NE
#endif  // XSL_IO_DEF
