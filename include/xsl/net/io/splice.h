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
#  include "xsl/byte.h"
#  include "xsl/coro.h"
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
// template <class From, class To>
// Task<void> splice(From& from, To& to, std::string& buffer) {
//   while (true) {
//     auto res = co_await from.read(std::as_writable_bytes(std::span(buffer)));
//     if (!res) {
//       co_return;
//     }
//     res = co_await to.write(std::as_bytes(std::span(buffer).subspan(0, res.size)));
//     if (!res) {
//       co_return;
//     }
//   }
// }
XSL_NET_IO_NE
#endif
