/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.13
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include "xsl/feature.h"
#  include "xsl/io/dyn.h"
#  include "xsl/sys/dev.h"
#  include "xsl/sys/net/conn.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/io.h"
#  include "xsl/type_traits.h"

#  include <sys/socket.h>

#  include <expected>
XSL_SYS_NET_NB
using namespace xsl::io;

template <class Traits, class Base>
class SocketBase : public Traits, public Base {
public:
  using Base::Base;

  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;

  using value_type = byte;

  SocketBase(int family, int type, int protocol,
             SocketAttribute attr = SocketAttribute::NonBlocking | SocketAttribute::CloseOnExec)
      : Traits(family, type, protocol),
        Base(::socket(family, type | static_cast<int>(attr), protocol)) {}

  explicit SocketBase(SocketAttribute attr
                      = SocketAttribute::NonBlocking | SocketAttribute::CloseOnExec)
      : Traits(),
        Base(::socket(this->family(), this->type() | static_cast<int>(attr), this->protocol())) {}
};

template <class Traits, class Base>
class AsyncSocketBase : public Traits, public Base {
public:
  using Base::Base;

  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;

  using value_type = byte;
  AsyncSocketBase(Poller &poller, int family, int type, int protocol,
                  SocketAttribute attr
                  = SocketAttribute::NonBlocking | SocketAttribute::CloseOnExec)
      : Traits(family, type, protocol),
        Base(::socket(family, type | static_cast<int>(attr), protocol), poller,
             typename Traits::poll_traits_type{}) {}

  explicit AsyncSocketBase(Poller &poller, SocketAttribute attr = SocketAttribute::NonBlocking
                                                                  | SocketAttribute::CloseOnExec)
      : Traits(),
        Base(::socket(this->family(), this->type() | static_cast<int>(attr), this->protocol()),
             poller, typename Traits::poll_traits_type{}) {}
};

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

/// @brief Read-only socket device
template <class Traits>
class ReadSocket : public SocketBase<Traits, RawReadDevice> {
public:
  using Base = SocketBase<Traits, RawReadDevice>;
  using async_type = AsyncReadSocket<Traits>;
  using Base::Base;
};
/// @brief Write-only socket device
template <class Traits>
class WriteSocket : public SocketBase<Traits, RawWriteDevice> {
public:
  using Base = SocketBase<Traits, RawWriteDevice>;
  using async_type = AsyncWriteSocket<Traits>;
  using Base::Base;
};
/// @brief Read-write socket device
template <class Traits>
class ReadWriteSocket : public SocketBase<Traits, RawReadWriteDevice>,
                        public ConnectionUtils<Traits> {
public:
  using Base = SocketBase<Traits, RawReadWriteDevice>;
  using async_type = AsyncReadWriteSocket<Traits>;
  using Base::Base;
  /**
   * @brief check and upgrade socket
   *
   * @param family
   * @param type
   * @param protocol
   * @return errc
   */
  constexpr errc check_and_upgrade(int family, int type, int protocol) {
    if (this->family() != family || this->type() != type || this->protocol() != protocol) {
      /// change socket attributes to arguments
      ::close(this->raw());
      this->raw() = ::socket(family, type, protocol);
      return check_ec(this->raw());
    }
    return {};
  }
};
/// @brief Async read-only socket device
template <class Traits>
class AsyncReadSocket : public AsyncSocketBase<Traits, RawAsyncReadDevice>, public NetAsyncRx {
public:
  using Base = AsyncSocketBase<Traits, RawAsyncReadDevice>;

  using io_dyn_chains = xsl::_n<AsyncReadSocket, io::DynAsyncReadDevice<AsyncReadSocket>>;

  using Base::Base;
};
/// @brief Async write-only socket device
template <class Traits>
class AsyncWriteSocket : public AsyncSocketBase<Traits, RawAsyncWriteDevice>, public NetAsyncTx {
public:
  using Base = AsyncSocketBase<Traits, RawAsyncWriteDevice>;

  template <template <class> class InOut = InOut>
  using rebind = AsyncSocketCompose<InOut<Traits>>::type;

  using Base::Base;
};
/// @brief Async read-write socket device
template <class Traits>
class AsyncReadWriteSocket : public AsyncSocketBase<Traits, RawAsyncReadWriteDevice>,
                             public NetAsyncRx,
                             public NetAsyncTx,
                             public ConnectionUtils<Traits> {
  using Base = AsyncSocketBase<Traits, RawAsyncReadWriteDevice>;

public:
  using Base::Base;

  template <template <class> class InOut = InOut>
  using rebind = AsyncSocketCompose<InOut<Traits>>::type;

  using sync_type = ReadWriteSocket<Traits>;
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

/**
 * @brief determine the socket type
 *
 * @tparam Flags, can be Tcp<Ip<4>>, Tcp<Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using Socket = SocketCompose<InOut<SocketTraits<Flags...>>>::type;

template <class Socket>
constexpr std::expected<Socket, errc> make_socket(int sock_attr) {
  typename Socket::traits_type socket_traits;
  int fd = ::socket(socket_traits.family(), socket_traits.type() | sock_attr,
                    socket_traits.protocol());
  if (fd < 0) {
    return std::unexpected{errc(errno)};
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
  using in_dev_type = _sys::net::AsyncReadSocket<Traits>;

  static constexpr Task<Result> read(in_dev_type &dev, std::span<byte> buf) {
    return dev.recv(buf);
  }
};

template <class Traits>
struct AIOTraits<_sys::net::AsyncWriteSocket<Traits>> {
  using value_type = byte;
  using out_dev_type = _sys::net::AsyncWriteSocket<Traits>;

  static Task<Result> write(out_dev_type &dev, std::span<const byte> buf) { return dev.send(buf); }

  static constexpr Task<Result> write_file(out_dev_type &dev, io::WriteFileHint &&hint) {
    return dev.send_file(std::move(hint));
  }
};

template <class Traits>
struct AIOTraits<_sys::net::AsyncReadWriteSocket<Traits>> {
  using value_type = byte;
  using io_dev_type = _sys::net::AsyncReadWriteSocket<Traits>;
  using in_dev_type = io_dev_type::template rebind<In>;
  using out_dev_type = io_dev_type::template rebind<Out>;

  static constexpr Task<Result> read(io_dev_type &dev, std::span<byte> buf) {
    return dev.recv(buf);
  }

  static constexpr Task<Result> write(io_dev_type &dev, std::span<const byte> buf) {
    return dev.send(buf);
  }
};
XSL_IO_NE
#endif
