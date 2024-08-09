#pragma once
#ifndef XSL_NET_IO_SPLICE
#  define XSL_NET_IO_SPLICE
#  include "xsl/ai/dev.h"
#  include "xsl/coro/lazy.h"
#  include "xsl/net/io/def.h"
XSL_NET_IO_NB
template <class Executor = coro::ExecutorBase, ai::AsyncReadDeviceLike<std::byte> From,
          ai::AsyncWriteDeviceLike<std::byte> To>
coro::Lazy<void> splice(From& from, To& to, std::string& buffer) {
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
