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
    LOG5("{} recv {} bytes", _raw, n);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {errc::not_connected}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      LOG6("no more data to read, waiting for signal");
      if (!co_await sig) {
        co_return {0, {errc::not_connected}};
      }
    } else {
      LOG6("rh {} recv error: {}", _raw, std::make_error_code(errc(errno)).message());
      co_return {0, {errc(errno)}};
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
        co_return {0, {errc::not_connected}};
      }
    } else {
      LOG6("recv error: {}", std::make_error_code(errc(errno)).message());
      co_return {0, {errc(errno)}};
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
    ssize_t n = ::recvfrom(_raw, buf.data(), buf.size(), 0, &sockaddr, &addrlen);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {errc::not_connected}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, {errc::not_connected}};
      }
    } else {
      co_return {0, {errc(errno)}};
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
        co_return {0, {errc::not_connected}};
      }
    } else {
      co_return {0, {errc(errno)}};
    }
  } while (true);
}

struct NetAsyncRx {
  Task<io::Result> recv(this auto &&self, std::span<byte> buf) {
    if constexpr (self.is_connection_based()) {
      return net::recv(self.raw(), buf, self.read_signal());
    } else {
      return imm_recv(self.raw(), buf, self.read_signal());
    }
  }
  template <class Traits, SocketTraitsCompatible<Traits> Up>
  Task<io::Result> recvfrom(this AsyncReadWriteSocket<Traits> &self, std::span<byte> buf,
                            SockAddr<Up> &addr) {
    if constexpr (self.is_connection_based()) {
      return net::recvfrom(self.raw(), buf, addr, self.read_signal());
    } else {
      return imm_recvfrom(self.raw(), buf, addr, self.read_signal());
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
        co_return {total, {errc::not_connected}};
      }
    } else {
      co_return {total, {errc(errno)}};
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
        co_return {total, {errc::not_connected}};
      }
    } else {
      co_return {total, {errc(errno)}};
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
    co_return io::Result{0, {errc(errno)}};
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
      co_return io::Result{static_cast<std::size_t>(offset), {errc::no_message}};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (static_cast<std::size_t>(offset) != hint.size) {
        break;
      }
      if (!co_await sig) {
        co_return io::Result{static_cast<std::size_t>(offset), {errc::not_connected}};
      }
    } else {
      co_return io::Result{static_cast<std::size_t>(offset), {errc(errno)}};
    }
  } while (hint.size != static_cast<std::size_t>(offset));
  co_return io::Result{static_cast<std::size_t>(offset), std::nullopt};
}

struct NetAsyncTx {
  /// @brief Send data to a device
  Task<io::Result> send(this auto &&self, std::span<const byte> data) {
    return net::send(self.raw(), data, self.write_signal());
  }
  /// @brief Send data to a specific address through a device
  template <class Traits, SocketTraitsCompatible<Traits> Up>
  Task<io::Result> sendto(this AsyncReadWriteSocket<Traits> &self, std::span<const byte> data,
                          SockAddr<Up> &addr) {
    return net::sendto(self.raw(), data, addr, self.write_signal());
  }
  /// @brief write file to device
  Task<io::Result> send_file(this auto &&self, io::WriteFileHint &&hint) {
    return net::send_file(self.raw(), std::move(hint), self.write_signal());
  }
};

XSL_SYS_NET_NE
#endif
