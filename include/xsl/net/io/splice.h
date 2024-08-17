#pragma once
#ifndef XSL_NET_IO_SPLICE
#  define XSL_NET_IO_SPLICE
#  include "xsl/ai.h"
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
 * @return Lazy<void>
 */
template <class Executor = coro::ExecutorBase, ai::ABRL From, ai::ABWL To>
Lazy<void, Executor> splice(From& from, To& to, std::string& buffer) {
  while (true) {
    auto [sz, err]
        = co_await from.template read<Executor>(xsl::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return;
    }
    auto [s_sz, s_err]
        = co_await to.template write<Executor>(xsl::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      co_return;
    }
  }
}
XSL_NET_IO_NE
#endif
