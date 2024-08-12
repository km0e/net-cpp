#pragma once
#ifndef XSL_NET_IO_SPLICE
#  define XSL_NET_IO_SPLICE
#  include "xsl/ai/dev.h"
#  include "xsl/coro.h"
#  include "xsl/net/io/def.h"
XSL_NET_IO_NB
/**
 * @brief splice data from one device to another
 *
 * @tparam Executor the executor type
 * @tparam From the device type to read data from
 * @tparam To the device type to write data to
 * @param from the device to read data from
 * @param to the device to write data to
 * @param buffer the buffer used to store the temporary data
 * @return coro::Lazy<void>
 */
template <class Executor = coro::ExecutorBase, ai::AsyncReadDeviceLike<std::byte> From,
          ai::AsyncWriteDeviceLike<std::byte> To>
coro::Lazy<void, Executor> splice(From& from, To& to, std::string& buffer) {
  while (true) {
    auto [sz, err]
        = co_await from.template read<Executor>(std::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return;
    }
    auto [s_sz, s_err]
        = co_await to.template write<Executor>(std::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      co_return;
    }
  }
}
XSL_NET_IO_NE
#endif
