#pragma once
#ifndef XSL_SYS_NET_IO
#  define XSL_SYS_NET_IO
#  include "xsl/ai.h"
#  include "xsl/logctl.h"
#  include "xsl/sys/net/def.h"

#  include <sys/socket.h>

#  include <cstddef>
#  include <cstring>
#  include <optional>
#  include <span>
#  include <system_error>
XSL_SYS_NET_NB
template <class Executor = coro::ExecutorBase, ai::ABRL S>
Task<Result, Executor> imm_recv(S &skt, std::span<byte> buf) {
  ssize_t n;
  size_t offset = 0;
  do {
    n = ::recv(skt.raw(), buf.data() + offset, buf.size() - offset, 0);
    LOG6("{} recv {} bytes", skt.raw(), n);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset == 0) {
        co_return Result(offset, {std::errc::no_message});
      }
      co_return Result(offset, std::nullopt);
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        co_return Result(offset, std::nullopt);
      }
      auto &sem = [&] -> auto & {
        if constexpr (ai::ABRWL<S>) {
          return skt.read_sem();//TODO: rename to poll
        } else {
          return skt.sem();
        }
      }();
      if (!co_await sem) {
        co_return Result(offset, {std::errc::not_connected});
      }
    } else {
      co_return Result(offset, {std::errc(errno)});
    }
  } while (offset < buf.size());
}

template <class Executor = coro::ExecutorBase, ai::ABWL S>
Task<Result, Executor> imm_send(S &skt, std::span<const byte> data) {
  while (true) {
    ssize_t n = ::send(skt.raw(), data.data(), data.size(), 0);
    if (static_cast<size_t>(n) == data.size()) {
      LOG5("send {} bytes", n);
      co_return {n, std::nullopt};
    }
    if (n > 0) {
      data = data.subspan(n);
      continue;
    }
    if (n == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
      co_return Result{0, {std::errc(errno)}};
    }
    auto &sem = [&] -> auto & {
      if constexpr (ai::ABRWL<S>) {
        return skt.write_sem();
      } else {
        return skt.sem();
      }
    }();
    if (!co_await sem) {
      co_return Result{0, {std::errc::not_connected}};
    }
  }
}

template <class Executor = coro::ExecutorBase, AsyncRawDeviceLike S, class SockAddr>
Task<Result, Executor> imm_sendto(S &skt, std::span<const byte> data, SockAddr &addr) {
  auto [sockaddr, addrlen] = addr.raw();
  while (true) {
    ssize_t n = ::sendto(skt.raw(), data.data(), data.size(), 0, sockaddr, *addrlen);
    if (static_cast<size_t>(n) == data.size()) {
      LOG5("sendto {} bytes", n);
      co_return {n, std::nullopt};
    }
    if (n > 0) {
      data = data.subspan(n);
      continue;
    }
    if (n == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
      LOG5("sendto args: {}, {}", skt.raw(),
           std::string_view{reinterpret_cast<const char *>(data.data()), data.size()});
      co_return Result{0, {std::errc(errno)}};
    }
    if (!co_await skt.sem()) {
      co_return Result{0, {std::errc::not_connected}};
    }
  }
}

template <class Executor = coro::ExecutorBase, AsyncRawDeviceLike S, class SockAddr>
Task<Result, Executor> imm_recvfrom(S &skt, std::span<byte> buf, SockAddr &addr) {
  auto [sockaddr, addrlen] = addr.raw();
  std::size_t total = 0;
  do {
    auto n = ::recvfrom(skt.raw(), buf.data() + total, buf.size() - total, 0, sockaddr, addrlen);
    if (n > 0) {
      total += n;
    } else if (n == 0) {
      if (total == 0) {
        co_return Result{0, {std::errc::no_message}};
      }
      co_return Result{total, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (total != 0) {
        co_return Result{total, std::nullopt};
      }
      if (!co_await skt.sem()) {
        co_return Result{total, {std::errc::not_connected}};
      }
    } else {
      co_return Result{total, {std::errc(errno)}};
    }
  } while (total < buf.size());
}

XSL_SYS_NET_NE
#endif
