#include "xsl/net/io/def.h"
#include "xsl/net/io/splice.h"
#include "xsl/sys/net/io.h"
XSL_NET_IO_NB
coro::Lazy<void> splice(sys::io::AsyncReadDevice from, sys::io::AsyncWriteDevice to,
                        std::string buffer) {
  while (true) {
    auto [sz, err]
        = co_await sys::net::immediate_recv(from, std::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return;
    }
    auto [s_sz, s_err]
        = co_await sys::net::immediate_send(to, std::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      co_return;
    }
  }
}
XSL_NET_IO_NE
