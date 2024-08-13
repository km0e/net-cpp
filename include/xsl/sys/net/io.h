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

struct SendfileHint {
  std::string
      path;  ///< file path, must be string, not string_view. Because this function will be called
             ///< in coroutine, and the path may be destroyed before the function is called.
  std::size_t offset;
  std::size_t size;
};
/**
 * @brief send file to socket
 *
 * @tparam Executor default is coro::ExecutorBase
 * @tparam S socket type
 * @param skt socket
 * @param hint sendfile hint
 * @return coro::Task<ai::Result, Executor>
 * @note The skt must keep alive until the task is finished, that is, the task and the socket must
 * have the same lifetime.
 */
template <class Executor = coro::ExecutorBase, AsyncSocketLike<feature::Out> S>
coro::Task<ai::Result, Executor> immediate_sendfile(S &skt, SendfileHint hint) {
  using Result = ai::Result;
  int ffd = open(hint.path.c_str(), O_RDONLY);
  if (ffd == -1) {
    LOG2("open file failed");
    co_return Result{0, {std::errc(errno)}};
  }
  sys::io::NativeDevice file{ffd};
  off_t offset = hint.offset;
  while (true) {
    ssize_t n = ::sendfile(skt.raw(), file.raw(), &offset, hint.size);
    // TODO: handle sendfile error
    if (n == static_cast<ssize_t>(hint.size)) {
      LOG6("{} send {} bytes file", skt.raw(), n);
      co_return Result{static_cast<std::size_t>(offset), std::nullopt};
    }
    if (n > 0) {
      LOG5("[sendfile] send {} bytes", n);
      hint.size -= n;
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
