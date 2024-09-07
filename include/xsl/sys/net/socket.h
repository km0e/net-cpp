/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.12
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include "xsl/feature.h"
#  include "xsl/io.h"
#  include "xsl/sys/dev.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/io.h"
#  include "xsl/type_traits.h"

#  include <sys/socket.h>

#  include <expected>
XSL_SYS_NET_NB
using namespace xsl::io;

template <class Traits>
class AsyncReadSocket;

template <class Traits>
class AsyncWriteSocket;

template <class Traits>
class AsyncReadWriteSocket;

template <class... Flags>
struct SocketSelector;

template <class... Flags>
struct AsyncSocketSelector;

/// @brief Read-only socket device
template <class Traits>
class ReadSocket : public RawReadDevice {
public:
  using Base = RawReadDevice;
  using async_type = AsyncReadSocket<Traits>;
  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;
  using Base::Base;
};
/// @brief Write-only socket device
template <class Traits>
class WriteSocket : public RawWriteDevice {
public:
  using Base = RawWriteDevice;
  using async_type = AsyncWriteSocket<Traits>;
  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;
  using Base::Base;
};
/// @brief Read-write socket device
template <class Traits>
class ReadWriteSocket : public Traits, public RawReadWriteDevice, public NetRxTraits {
public:
  using Base = RawReadWriteDevice;
  using async_type = AsyncReadWriteSocket<Traits>;
  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;
  using Base::Base;
};
/// @brief Async read-only socket device
template <class Traits>
class AsyncReadSocket : public Traits, public RawAsyncReadDevice, public NetAsyncRxTraits {
public:
  using Base = RawAsyncReadDevice;
  using value_type = byte;
  using traits_type = Traits;
  using dynamic_type = AsyncReadSocket;
  using io_dyn_chains = xsl::_n<AsyncReadSocket, DynAsyncReadDevice<AsyncReadSocket>>;

  using Base::Base;
};
/// @brief Async write-only socket device
template <class Traits>
class AsyncWriteSocket : public Traits, public RawAsyncWriteDevice, public NetAsyncTxTraits {
public:
  using Base = RawAsyncWriteDevice;
  using value_type = byte;

  using traits_type = Traits;
  using dynamic_type = AsyncWriteSocket;
  using io_dyn_chains = xsl::_n<AsyncWriteSocket, DynAsyncWriteDevice<AsyncWriteSocket>>;

  using Base::Base;
};
/// @brief Async read-write socket device
template <class Traits>
class AsyncReadWriteSocket : public Traits,
                             public RawAsyncReadWriteDevice,
                             public NetAsyncRxTraits,
                             public NetAsyncTxTraits {
public:
  using Base = RawAsyncReadWriteDevice;

  using traits_type = Traits;

  using value_type = byte;

  using Base::Base;
  template <template <class> class InOut = InOut>
  using rebind = AsyncSocketSelector<InOut<Traits>>::type;
};

template <class... Flags>
struct SocketSelector;

template <class Traits>
struct SocketSelector<In<Traits>> {
  using type = ReadSocket<Traits>;
};

template <class Traits>
struct SocketSelector<Out<Traits>> {
  using type = WriteSocket<Traits>;
};

template <class Traits>
struct SocketSelector<InOut<Traits>> {
  using type = ReadWriteSocket<Traits>;
};

template <class... Flags>
using SocketCompose
    = organize_feature_flags_t<SocketSelector<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>,
                               Flags...>;

template <class Traits>
struct AsyncSocketSelector<In<Traits>> {
  using type = AsyncReadSocket<Traits>;
};

template <class Traits>
struct AsyncSocketSelector<Out<Traits>> {
  using type = AsyncWriteSocket<Traits>;
};

template <class Traits>
struct AsyncSocketSelector<InOut<Traits>> {
  using type = AsyncReadWriteSocket<Traits>;
};

template <class... Flags>
using AsyncSocketCompose = organize_feature_flags_t<
    AsyncSocketSelector<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>, Flags...>;

/**
 * @brief determine the socket type
 *
 * @tparam Flags, can be Tcp<Ip<4>>, Tcp<Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using Socket = SocketCompose<InOut<SocketTraits<Flags...>>>::type;

template <class Socket>
constexpr std::expected<Socket, std::errc> make_socket() {
  typename Socket::traits_type socket_traits;
  int fd = ::socket(socket_traits.family(), socket_traits.type(), socket_traits.protocol());
  if (fd < 0) {
    return std::unexpected{std::errc(errno)};
  }
  return Socket{fd};
}
/**
 * @brief determine the socket type asynchronously
 *
 * @tparam Flags, can be Tcp<Ip<4>>, Tcp<Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using AsyncSocket = AsyncSocketCompose<InOut<SocketTraits<Flags...>>>::type;
XSL_SYS_NET_NE
XSL_IO_NB
template <class Traits>
struct IOTraits<_sys::net::ReadSocket<Traits>> {
  using value_type = byte;
  using device_type = _sys::net::ReadSocket<Traits>;

  static io::Result read(device_type &, std::span<byte>) { std::unreachable(); }
};

template <class Traits>
struct IOTraits<_sys::net::WriteSocket<Traits>> {
  using value_type = byte;
  using device_type = _sys::net::WriteSocket<Traits>;

  static io::Result write(device_type &, std::span<const byte>) { std::unreachable(); }
};

template <class Traits>
struct IOTraits<_sys::net::ReadWriteSocket<Traits>> {
  using value_type = byte;
  using device_type = _sys::net::ReadWriteSocket<Traits>;

  static io::Result read(device_type &, std::span<byte>) { std::unreachable(); }

  static io::Result write(device_type &, std::span<const byte>) { std::unreachable(); }
};

template <class Traits>
struct AIOTraits<_sys::net::AsyncReadSocket<Traits>> {
  using value_type = byte;
  using device_type = _sys::net::AsyncReadSocket<Traits>;

  static constexpr Task<Result> read(device_type &dev, std::span<byte> buf) {
    return dev.recv(buf);
  }
};

template <class Traits>
struct AIOTraits<_sys::net::AsyncWriteSocket<Traits>> {
  using value_type = byte;
  using device_type = _sys::net::AsyncWriteSocket<Traits>;

  static Task<Result> write(device_type &dev, std::span<const byte> buf) { return dev.send(buf); }

  static constexpr Task<Result> write_file(device_type &dev, io::WriteFileHint &&hint) {
    return dev.send_file(std::move(hint));
  }
};

template <class Traits>
struct AIOTraits<_sys::net::AsyncReadWriteSocket<Traits>> {
  using value_type = byte;
  using device_type = _sys::net::AsyncReadWriteSocket<Traits>;

  static constexpr Task<Result> read(device_type &dev, std::span<byte> buf) {
    return dev.recv(buf);
  }

  static constexpr Task<Result> write(device_type &dev, std::span<const byte> buf) {
    return dev.send(buf);
  }
};
XSL_IO_NE
#endif
