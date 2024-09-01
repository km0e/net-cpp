/**
 * @file splice.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Splice the data from one device to another
 * @version 0.12
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_IO_SPLICE
#  define XSL_NET_IO_SPLICE
#  include "xsl/coro.h"
#  include "xsl/io.h"
#  include "xsl/net/io/def.h"
XSL_NET_IO_NB
/**
 * @brief Splice the data from one device to another
 *
 * @tparam From the source device
 * @tparam To the destination device
 * @param from the source device
 * @param to the destination device
 * @param buffer the buffer
 * @return Task<void>
 */
template <class From, class To>
Task<void> splice(From& from, To& to, std::string& buffer) {
  while (true) {
    auto [sz, err]
        = co_await AIOTraits<From>::read(from, xsl::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return;
    }
    auto [s_sz, s_err]
        = co_await AIOTraits<To>::write(to, xsl::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      co_return;
    }
  }
}
XSL_NET_IO_NE
#endif
