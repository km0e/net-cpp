/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_DEF
#  define XSL_ASIO_DEF

#  define XSL_ASIO_NB \
    XSL_NB            \
    namespace _asio {
#  define XSL_ASIO_NE \
    XSL_NE            \
    }  // namespace _asio

#  include <xsl/byte.h>
#  include <xsl/coro/def.h>
#  include <xsl/def.h>
#  include <xsl/io/def.h>

XSL_ASIO_NB

using io::Result;

/**
 * @brief Define the abstract async read device
 */
template <class Device>
concept AsyncRead = requires(Device t, xsl::byte* data, std::size_t size) {
  { t.read(data, size) } -> coro::Awaitable;
  requires std::same_as<typename decltype(t.read(data, size))::result_type, Result>;
};

/**
 * @brief Define the abstract async write device
 */
template <class Device>
concept AsyncWrite = requires(Device t, const byte* data, std::size_t size) {
  { t.write(data, size) } -> coro::Awaitable;
  requires std::same_as<typename decltype(t.write(data, size))::result_type, Result>;
};

/**
 * @brief Define the abstract async read and write device
 * @note This concept requires both AsyncRead and AsyncWrite to be satisfied
 */
template <class Device>
concept AsyncReadWrite = AsyncRead<Device> && AsyncWrite<Device>;

XSL_ASIO_NE
#endif
