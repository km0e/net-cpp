/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief IO utilities
 * @version 0.11
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_IO
#  define XSL_SYS_NET_IO
#  include "xsl/coro.h"
#  include "xsl/io/def.h"
#  include "xsl/logctl.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/sockaddr.h"

#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <sys/socket.h>

#  include <cassert>
#  include <cstddef>
#  include <cstring>
#  include <optional>
#  include <span>
#  include <system_error>
#  include <type_traits>
XSL_SYS_NET_NB

/**
 * @brief Receive data from a device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer>
Task<io::Result> recv(RawHandle _raw, std::span<byte> buf, AnySignal<SignalTraits, Pointer> &sig) {
  assert(buf.size() > 0);
  do {
    ssize_t n = ::recv(_raw, buf.data(), buf.size(), 0);
    LOG6("{} recv {} bytes", _raw, n);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {std::errc::bad_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      LOG6("no more data to read, waiting for signal");
      if (!co_await sig) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      LOG6("recv error: {}", std::make_error_code(std::errc(errno)).message());
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}
/**
 * @brief Receive data from a device
 *
 * @tparam Dev the device type
 * @param dev the device
 * @param buf the buffer
 * @return Task<io::Result>
 */
template <class Dev>
constexpr Task<io::Result> recv(Dev &dev, std::span<byte> buf) {
  return io::AIOTraits<Dev>::recv(dev, buf);
}
/**
 * @brief Receive data from a device, specialized for not connect-based device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer>
Task<io::Result> imm_recv(RawHandle _raw, std::span<byte> buf,
                          AnySignal<SignalTraits, Pointer> &sig) {
  do {
    ssize_t n = ::recv(_raw, buf.data(), buf.size(), 0);
    LOG6("{} recv {} bytes", _raw, n);
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      LOG6("recv error: {}", std::make_error_code(std::errc(errno)).message());
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}
/**
 * @brief Receive data from any address
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @tparam SockAddr the socket address type
 * @param _raw the raw handle
 * @param buf the buffer
 * @param addr the address
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer, class SockAddr>
Task<io::Result> recvfrom(RawHandle _raw, std::span<byte> buf, SockAddr &addr,
                          AnySignal<SignalTraits, Pointer> &sig) {
  assert(buf.size() > 0);
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::recvfrom(_raw, buf.data(), buf.size(), 0, sockaddr, addrlen);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {std::errc::not_connected}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}
/**
 * @brief Receive data from any address
 *
 * @tparam Dev the device type
 * @tparam SockAddr the socket address type
 * @param dev the device
 * @param buf the buffer
 * @param addr the address
 * @return Task<io::Result>
 */
template <class Dev, class SockAddr>
constexpr Task<io::Result> recvfrom(Dev &dev, std::span<byte> buf, SockAddr &addr) {
  return io::AIOTraits<Dev>::recvfrom(dev, buf, addr);
}
/**
 * @brief Receive data from any address, specialized for not connect-based device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @tparam SockAddr the socket address type
 * @param _raw the raw handle
 * @param buf the buffer
 * @param addr the address
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer, class SockAddr>
Task<io::Result> imm_recvfrom(RawHandle _raw, std::span<byte> buf, SockAddr &addr,
                              AnySignal<SignalTraits, Pointer> &sig) {
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::recvfrom(_raw, buf.data(), buf.size(), 0, &sockaddr, &addrlen);
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, {std::errc::not_connected}};
      }
    } else {
      co_return {0, {std::errc(errno)}};
    }
  } while (true);
}
struct NetRxTraits {
  template <class Self>
    requires(std::decay_t<Self>::is_connection_based())
  std::errc listen(this Self &&self, int max_connections = 128) {
    if (::listen(self.raw(), max_connections) == -1) {
      return std::errc{errno};
    }
    return {};
  }
  template <class Self>
    requires(!requires { std::decay_t<Self>::is_connection_based(); })
  [[nodiscard("must check if the operation is successful")]] std::errc listen(this Self &&self,
                                                                              int max_connections
                                                                              = 128) {
    if (!self.is_connection_based()) {
      return std::errc::operation_not_supported;
    }
    if (::listen(self.raw(), max_connections) == -1) {
      return std::errc{errno};
    }
    return {};
  }
  template <class Self, class Decayed = std::decay_t<Self>, class Traits = Decayed::traits_type>
    requires(Decayed::is_connection_based())
  constexpr std::expected<ReadWriteSocket<Traits>, std::errc> accept(this Self &&self,
                                                                     SockAddr<Traits> *addr
                                                                     = nullptr) {
    auto tmp_fd = [&self, addr] {
      if (addr == nullptr) {
        return ::accept4(self.raw(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
      } else {
        auto [sockaddr, addrlen] = addr->raw();
        return ::accept4(self.raw(), &sockaddr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
      }
    }();
    if (tmp_fd < 0) {
      return std::unexpected{std::errc(errno)};
    }
    LOG5("accept socket {}", tmp_fd);
    // char ip[NI_MAXHOST], port[NI_MAXSERV];
    // if (getnameinfo(&addr, addrlen, ip, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST |
    // NI_NUMERICSERV)
    //     != 0) {
    //   return std::unexpected{std::errc(errno)};
    // }
    return ReadWriteSocket<Traits>(tmp_fd);
  }
};
struct NetAsyncRxTraits : public NetRxTraits {
  Task<io::Result> recv(this auto &&self, std::span<byte> buf) {
    if constexpr (self.is_connection_based()) {
      return net::recv(self.raw(), buf, self.read_signal());
    } else {
      return imm_recv(self.raw(), buf, self.read_signal());
    }
  }
  template <class SockAddr>
  Task<io::Result> recvfrom(this auto &&self, std::span<byte> buf, SockAddr &addr) {
    if constexpr (self.is_connection_based()) {
      return net::recvfrom(self.raw(), buf, addr, self.read_signal());
    } else {
      return imm_recvfrom(self.raw(), buf, addr, self.read_signal());
    }
  }
  template <class Self, class Decayed = std::decay_t<Self>, class Traits = Decayed::traits_type>
    requires(Decayed::is_connection_based())
  Task<std::expected<ReadWriteSocket<Traits>, std::errc>> accept(this Self &&self,
                                                                 SockAddr<Traits> *addr = nullptr) {
    while (true) {
      auto res = self.NetRxTraits::accept(addr);
      if (res) {
        co_return std::move(*res);
      } else if (res.error() == std::errc::resource_unavailable_try_again
                 || res.error() == std::errc::operation_would_block) {
        if (!co_await self.read_signal()) {
          co_return std::unexpected{std::errc::not_connected};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }
};
/**
 * @brief Send data to a device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param data the data to send
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer>
Task<io::Result> send(RawHandle _raw, std::span<const byte> data,
                      AnySignal<SignalTraits, Pointer> &sig) {
  std::size_t total = 0;
  do {
    ssize_t n = ::send(_raw, data.data(), data.size(), 0);
    DEBUG("send {} bytes", n);
    if (n > 0) {
      data = data.subspan(n);
      total += n;
    } else if (n == 0) {
      break;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {total, {std::errc::not_connected}};
      }
    } else {
      co_return {total, {std::errc(errno)}};
    }
  } while (!data.empty());
  co_return {total, std::nullopt};
}
/**
 * @brief Send data to a device
 *
 * @tparam Dev the device type
 * @param dev the device
 * @param data the data to send
 * @return Task<io::Result>
 */
template <class Dev>
constexpr Task<io::Result> send(Dev &dev, std::span<const byte> data) {
  return io::AIOTraits<Dev>::send(dev, data);
}
/**
 * @brief Send data to a specific address through a device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @tparam SockAddr the socket address type
 * @param _raw the raw handle
 * @param data the data to send
 * @param addr the address to send
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer, class SockAddr>
Task<io::Result> sendto(RawHandle _raw, std::span<const byte> data, SockAddr &addr,
                        AnySignal<SignalTraits, Pointer> &sig) {
  auto [sockaddr, addrlen] = addr.raw();
  std::size_t total = 0;
  do {
    ssize_t n = ::sendto(_raw, data.data(), data.size(), 0, &sockaddr, addrlen);
    if (n >= 0) {
      data = data.subspan(n);
      total += n;
    } else if (n == 0) {
      break;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {total, {std::errc::not_connected}};
      }
    } else {
      co_return {total, {std::errc(errno)}};
    }
  } while (!data.empty());
  co_return {total, std::nullopt};
}
/**
 * @brief Send data to a specific address through a device
 *
 * @tparam Dev the device type
 * @tparam SockAddr the socket address type
 * @param dev the device
 * @param data the data to send
 * @param addr the address to send
 * @return Task<io::Result>
 */
template <class Dev, class SockAddr>
constexpr Task<io::Result> sendto(Dev &dev, std::span<const byte> data, SockAddr &addr) {
  return io::AIOTraits<Dev>::sendto(dev, data, addr);
}

/**
 * @brief write file to device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param hint the hint to write file
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class SignalTraits, class Pointer>
Task<io::Result> send_file(RawHandle _raw, io::WriteFileHint hint,
                           AnySignal<SignalTraits, Pointer> &sig) {
  int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
  if (ffd == -1) {
    LOG2("open file failed");
    co_return io::Result{0, {std::errc(errno)}};
  }
  Defer defer{[ffd] { close(ffd); }};
  off_t offset = hint.offset;
  do {
    ssize_t n = ::sendfile(_raw, ffd, &offset, hint.size - offset);
    if (n > 0) {
      LOG5("[sendfile] send {} bytes", n);
      offset += n;
    } else if (n == 0) {
      LOG6("{} send {} bytes file", _raw, n);
      if (static_cast<std::size_t>(offset) != hint.size) {
        break;
      }
      co_return io::Result{static_cast<std::size_t>(offset), {std::errc::no_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (static_cast<std::size_t>(offset) != hint.size) {
        break;
      }
      if (!co_await sig) {
        co_return io::Result{static_cast<std::size_t>(offset), {std::errc::not_connected}};
      }
    } else {
      co_return io::Result{static_cast<std::size_t>(offset), {std::errc(errno)}};
    }
  } while (hint.size != static_cast<std::size_t>(offset));
  co_return io::Result{static_cast<std::size_t>(offset), std::nullopt};
}

struct NetAsyncTxTraits {
  Task<io::Result> send(this auto &&self, std::span<const byte> data) {
    return net::send(self.raw(), data, self.write_signal());
  }
  template <class SockAddr>
  Task<io::Result> sendto(this auto &&self, std::span<const byte> data, SockAddr &addr) {
    return net::sendto(self.raw(), data, addr, self.write_signal());
  }
  Task<io::Result> send_file(this auto &&self, io::WriteFileHint &&hint) {
    return net::send_file(self.raw(), std::move(hint), self.write_signal());
  }
};

XSL_SYS_NET_NE
#endif
