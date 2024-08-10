#pragma once
#ifndef XSL_SYS_NET_IO
#  define XSL_SYS_NET_IO
#  include "xsl/ai/dev.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"
#  include "xsl/sys/io/dev.h"
#  include "xsl/sys/net/def.h"

#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <sys/stat.h>
#  include <sys/types.h>

#  include <cstddef>
#  include <filesystem>
#  include <optional>
#  include <system_error>
#  include <tuple>
XSL_SYS_NET_NB
template <class Executor = coro::ExecutorBase, AsyncSocketLike<feature::In> S>
coro::Task<ai::Result, Executor> immediate_recv(S &skt, std::span<std::byte> buf) {
  using Result = ai::Result;
  ssize_t n;
  size_t offset = 0;
  while (true) {
    n = ::recv(skt.raw(), buf.data() + offset, buf.size() - offset, 0);
    LOG5("recv n: {}", n);
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
      co_return Result(offset, std::nullopt);
    }
    LOG6("recv {} bytes", n);
    offset += n;
  };
  LOG5("end recv string");
  co_return std::make_tuple(offset, std::nullopt);
}

template <class Executor = coro::ExecutorBase, AsyncSocketLike<feature::Out> S>
coro::Task<ai::Result, Executor> immediate_send(S &skt, std::span<const std::byte> data) {
  using Result = ai::Result;
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
template <class Executor = coro::ExecutorBase, AsyncSocketLike<feature::Out> S>
coro::Task<ai::Result, Executor> immediate_sendfile(S &skt, std::filesystem::path path) {
  using Result = ai::Result;
  int ffd = open(path.c_str(), O_RDONLY);
  if (ffd == -1) {
    LOG2("open file failed");
    co_return Result{0, {std::errc(errno)}};
  }
  sys::io::NativeDevice file{ffd};
  struct stat st;
  if (fstat(file.raw(), &st) == -1) {
    LOG2("fstat failed");
    co_return Result{0, {std::errc(errno)}};
  }
  off_t offset = 0;
  while (true) {
    ssize_t n = ::sendfile(skt.raw(), file.raw(), &offset, st.st_size);
    // TODO: handle sendfile error
    if (n == st.st_size) {
      co_return Result{static_cast<std::size_t>(offset), std::nullopt};
    }
    if (n > 0) {
      LOG5("[sendfile] send {} bytes", n);
      st.st_size -= n;
      continue;
    }
    if ((n == -1) && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
      co_return Result{static_cast<std::size_t>(offset), {std::errc(errno)}};
    }

    if (!co_await skt.sem()) {
      co_return Result{static_cast<std::size_t>(offset), {std::errc::not_connected}};
    }
  }
}
XSL_SYS_NET_NE
#endif
