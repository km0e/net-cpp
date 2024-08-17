#pragma once
#ifndef XSL_SYS_NET_IO
#  define XSL_SYS_NET_IO
#  include "xsl/ai.h"
#  include "xsl/sys/net/def.h"

#  include <sys/socket.h>

#  include <cstring>
#  include <optional>
#  include <span>
#  include <system_error>
#  include <tuple>
XSL_SYS_NET_NB
template <class Executor = coro::ExecutorBase, AsyncSocketLike<feature::In> S>
Task<Result, Executor> immediate_recv(S &skt, std::span<byte> buf) {
  using Result = Result;
  ssize_t n;
  size_t offset = 0;
  while (true) {
    n = ::recv(skt.raw(), buf.data() + offset, buf.size() - offset, 0);
    LOG6("{} recv {} bytes", skt.raw(), n);
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (offset != 0) {
          LOG5("recv over");
          break;
        }
        LOG5("no data");
        if (!co_await skt.sem()) {
          co_return Result(offset, {std::errc::not_connected});
        }
        continue;
      } else {
        LOG2("Failed to recv data, err : {}", strerror(errno));
        // TODO: handle recv error
        co_return Result(offset, {std::errc(errno)});
      }
    } else if (n == 0) {
      LOG5("recv eof");
      if (offset == 0) {
        co_return Result(offset, {std::errc::no_message});
      }
      break;
    }
    offset += n;
  };
  LOG6("end recv string");
  LOG6("recv string:\n{}", std::string_view{reinterpret_cast<const char *>(buf.data()), offset});
  co_return std::make_tuple(offset, std::nullopt);
}

template <class Executor = coro::ExecutorBase, AsyncSocketLike<feature::Out> S>
Task<Result, Executor> immediate_send(S &skt, std::span<const byte> data) {
  using Result = Result;
  while (true) {
    ssize_t n = ::send(skt.raw(), data.data(), data.size(), 0);
    if (static_cast<size_t>(n) == data.size()) {
      co_return {n, std::nullopt};
    }
    if (n > 0) {
      data = data.subspan(n);
      continue;
    }
    if (n == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
      co_return Result{0, {std::errc(errno)}};
    }
    if (!co_await skt.sem()) {
      co_return Result{0, {std::errc::not_connected}};
    }
  }
}

XSL_SYS_NET_NE
#endif
