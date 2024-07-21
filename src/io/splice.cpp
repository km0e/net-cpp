#include "xsl/coro/lazy.h"
#include "xsl/io/def.h"
#include "xsl/io/splice.h"
#include "xsl/sys/io.h"
XSL_IO_NB
coro::Lazy<void> splice(sys::io::AsyncReadDevice from, sys::io::AsyncWriteDevice to,
                        std::string buffer) {
  while (true) {
    auto [sz, err]
        = co_await sys::io::immediate_read(from, std::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return;
    }
    auto res
        = co_await sys::io::immediate_write(to, std::as_bytes(std::span(buffer).subspan(0, sz)));
    if (!res) {
      co_return;
    }
  }
}
XSL_IO_NE
