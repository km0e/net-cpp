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
#  include "xsl/io.h"
#  include "xsl/logctl.h"
#  include "xsl/sys/net/def.h"

#  include <sys/sendfile.h>
#  include <sys/socket.h>

#  include <cassert>
#  include <cstring>
#  include <optional>
#  include <span>
#  include <system_error>
XSL_SYS_NET_NB
using RawHandle = int;
/**
 * @brief Receive data from a device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
template <class Pointer>
Task<io::Result> recv(RawHandle _raw, std::span<byte> buf, SignalReceiver<Pointer> &sig) {
  assert(buf.size() > 0);
  do {
    ssize_t n = ::recv(_raw, buf.data(), buf.size(), 0);
    LOG6("{} recv {} bytes", _raw, n);
    if (n > 0) {
      co_return {n, std::nullopt};
    } else if (n == 0) {
      co_return {0, {std::errc::not_connected}};
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
 * @brief Receive data from a device
 *
 * @tparam Dev the device type
 * @param dev the device
 * @param buf the buffer
 * @return Task<io::Result>
 */
template <class Dev>
Task<io::Result> recv(Dev &dev, std::span<byte> buf) {
  return AIOTraits<Dev>::recv(dev, buf);
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
template <class Pointer>
Task<io::Result> imm_recv(RawHandle _raw, std::span<byte> buf, SignalReceiver<Pointer> &sig) {
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
 * @brief Send data to a device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param data the data to send
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class Pointer>
Task<io::Result> send(RawHandle _raw, std::span<const byte> data, SignalReceiver<Pointer> &sig) {
  do {
    ssize_t n = ::send(_raw, data.data(), data.size(), 0);
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
/**
 * @brief Send data to a device
 *
 * @tparam Dev the device type
 * @param dev the device
 * @param data the data to send
 * @return Task<io::Result>
 */
template <class Dev>
Task<io::Result> send(Dev &dev, std::span<const byte> data) {
  return AIOTraits<Dev>::send(dev, data);
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
template <class Pointer, class SockAddr>
Task<io::Result> sendto(RawHandle _raw, std::span<const byte> data, SockAddr &addr,
                        SignalReceiver<Pointer> &sig) {
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::sendto(_raw, data.data(), data.size(), 0, sockaddr, *addrlen);
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
Task<io::Result> sendto(Dev &dev, std::span<const byte> data, SockAddr &addr) {
  return AIOTraits<Dev>::sendto(dev, data, addr);
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
template <class Pointer, class SockAddr>
Task<io::Result> recvfrom(RawHandle _raw, std::span<byte> buf, SockAddr &addr,
                          SignalReceiver<Pointer> &sig) {
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
Task<io::Result> recvfrom(Dev &dev, std::span<byte> buf, SockAddr &addr) {
  return AIOTraits<Dev>::recvfrom(dev, buf, addr);
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
template <class Pointer, class SockAddr>
Task<io::Result> imm_recvfrom(RawHandle _raw, std::span<byte> buf, SockAddr &addr,
                              SignalReceiver<Pointer> &sig) {
  auto [sockaddr, addrlen] = addr.raw();
  do {
    ssize_t n = ::recvfrom(_raw, buf.data(), buf.size(), 0, sockaddr, addrlen);
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

/**
 * @brief write file to device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param hint the hint to write file
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class Pointer>
Task<io::Result> write_file(RawHandle _raw, _sys::WriteFileHint hint,
                            SignalReceiver<Pointer> &sig) {
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

XSL_SYS_NET_NE
#endif
