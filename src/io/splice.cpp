#include "xsl/io/def.h"
#include "xsl/io/splice.h"
XSL_IO_NB
// Lazy<void> splice(sys::io::AsyncDevice<In> from, sys::io::AsyncDevice<Out> to,
//                         std::string buffer) {
//   while (true) {
//     auto [sz, err]
//         = co_await sys::io::immediate_recv(from, std::as_writable_bytes(std::span(buffer)));
//     if (err) {
//       co_return;
//     }
//     auto res
//         = co_await sys::io::immediate_send(to, std::as_bytes(std::span(buffer).subspan(0, sz)));
//     if (!res) {
//       co_return;
//     }
//   }
// }
XSL_IO_NE
