#pragma once
#ifndef XSL_SYS_NET_IO
#  define XSL_SYS_NET_IO
#  include "xsl/ai.h"
#  include "xsl/logctl.h"
#  include "xsl/sys/net/def.h"

#  include <sys/socket.h>

#  include <cassert>
#  include <cstring>
#  include <optional>
#  include <span>
#  include <system_error>
XSL_SYS_NET_NB
template <class Executor = coro::ExecutorBase, CSocket S>
Task<Result, Executor> imm_recv(S &skt, std::span<byte> buf) {
  assert(buf.size() > 0);
  do {
    ssize_t n = ::recv(skt.raw(), buf.data(), buf.size(), 0);
    LOG6("{} recv {} bytes", skt.raw(), n);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {std::errc::not_connected}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await skt.poll_for_read()) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      LOG6("recv error: {}", std::make_error_code(std::errc(errno)).message());
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}
template <class Executor = coro::ExecutorBase, ai::ABRL S>
  requires(!CSocket<S>)
Task<Result, Executor> imm_recv(S &skt, std::span<byte> buf) {
  do {
    ssize_t n = ::recv(skt.raw(), buf.data(), buf.size(), 0);
    LOG6("{} recv {} bytes", skt.raw(), n);
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await skt.poll_for_read()) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      LOG6("recv error: {}", std::make_error_code(std::errc(errno)).message());
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}

template <class Executor = coro::ExecutorBase, ai::ABWL S>
Task<Result, Executor> imm_send(S &skt, std::span<const byte> data) {
  do {
    ssize_t n = ::send(skt.raw(), data.data(), data.size(), 0);
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await skt.poll_for_write()) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}

template <class Executor = coro::ExecutorBase, ai::ABWL S, class SockAddr>
Task<Result, Executor> imm_sendto(S &skt, std::span<const byte> data, SockAddr &addr) {
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::sendto(skt.raw(), data.data(), data.size(), 0, sockaddr, *addrlen);
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await skt.poll_for_write()) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}

template <class Executor = coro::ExecutorBase, CSocket S, class SockAddr>
Task<Result, Executor> imm_recvfrom(S &skt, std::span<byte> buf, SockAddr &addr) {
  assert(buf.size() > 0);
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::recvfrom(skt.raw(), buf.data(), buf.size(), 0, sockaddr, addrlen);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {std::errc::not_connected}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await skt.poll_for_read()) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}

template <class Executor = coro::ExecutorBase, ai::ABRL S, class SockAddr>
  requires(!CSocket<S>)
Task<Result, Executor> imm_recvfrom(S &skt, std::span<byte> buf, SockAddr &addr) {
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::recvfrom(skt.raw(), buf.data(), buf.size(), 0, sockaddr, addrlen);
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await skt.poll_for_read()) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}

XSL_SYS_NET_NE
#endif
