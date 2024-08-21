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
  std::size_t offset = 0;
  do {
    ssize_t n = ::recv(skt.raw(), buf.data() + offset, buf.size() - offset, 0);
    LOG6("{} recv {} bytes", skt.raw(), n);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset != 0) {
        break;
      }
      co_return {offset, {std::errc::no_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        break;
      }
      if (!co_await skt.poll_for_read()) {
        co_return {offset, {std::errc::not_connected}};
      }
    } else {
      co_return {offset, {std::errc(errno)}};
    }
  } while (false);
  co_return {offset, std::nullopt};
}

template <class Executor = coro::ExecutorBase, ai::ABWL S>
Task<Result, Executor> imm_send(S &skt, std::span<const byte> data) {
  std::size_t offset = 0;
  do {
    ssize_t n = ::send(skt.raw(), data.data() + offset, data.size() - offset, 0);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset != 0) {
        break;
      }
      co_return {offset, {std::errc::no_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        break;
      }
      if (!co_await skt.poll_for_write()) {
        co_return {offset, {std::errc::not_connected}};
      }
    } else {
      co_return {offset, {std::errc(errno)}};
    }
  } while (false);
  co_return {offset, std::nullopt};
}

template <class Executor = coro::ExecutorBase, ai::ABWL S, class SockAddr>
Task<Result, Executor> imm_sendto(S &skt, std::span<const byte> data, SockAddr &addr) {
  auto [sockaddr, addrlen] = addr.raw();
  std::size_t offset = 0;
  do {
    ssize_t n
        = ::sendto(skt.raw(), data.data() + offset, data.size() - offset, 0, sockaddr, *addrlen);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset != 0) {
        break;
      }
      co_return {offset, {std::errc::no_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        break;
      }
      if (!co_await skt.poll_for_write()) {
        co_return {offset, {std::errc::not_connected}};
      }
    } else {
      co_return {offset, {std::errc(errno)}};
    }
  } while (false);
  co_return {offset, std::nullopt};
}

template <class Executor = coro::ExecutorBase, ai::ABRL S, class SockAddr>
Task<Result, Executor> imm_recvfrom(S &skt, std::span<byte> buf, SockAddr &addr) {
  auto [sockaddr, addrlen] = addr.raw();
  std::size_t offset = 0;
  do {
    ssize_t n
        = ::recvfrom(skt.raw(), buf.data() + offset, buf.size() - offset, 0, sockaddr, addrlen);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset != 0) {
        break;
      }
      co_return {offset, {std::errc::no_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        break;
      }
      if (!co_await skt.poll_for_read()) {
        co_return {offset, {std::errc::not_connected}};
      }
    } else {
      co_return {offset, {std::errc(errno)}};
    }
  } while (false);
  co_return {offset, std::nullopt};
}

XSL_SYS_NET_NE
#endif
