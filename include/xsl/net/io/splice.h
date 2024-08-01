#pragma once
#ifndef XSL_NET_IO_SPLICE
#  define XSL_NET_IO_SPLICE
#  include "xsl/coro/lazy.h"
#  include "xsl/feature.h"
#  include "xsl/net/io/def.h"
#  include "xsl/sys/io/dev.h"
#  include "xsl/sys/net/io.h"
XSL_NET_IO_NB
template <class Executor = coro::ExecutorBase>
coro::Lazy<void> splice(sys::io::AsyncDevice<feature::In<std::byte>> from,
                        sys::io::AsyncDevice<feature::Out<std::byte>> to, std::string buffer) {
  while (true) {
    auto [sz, err] = co_await sys::net::immediate_recv<Executor>(
        from, std::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return;
    }
    auto [s_sz, s_err] = co_await sys::net::immediate_send<Executor>(
        to, std::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      co_return;
    }
  }
}
XSL_NET_IO_NE
#endif
