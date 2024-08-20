#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include "xsl/feature.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/io.h"
#  include "xsl/sys/raw.h"

#  include <sys/socket.h>

#  include <expected>
XSL_SYS_NET_NB

namespace impl_sock {
  template <class... Flags>
  class Socket;

  template <class... Flags>
  using SocketCompose = feature::organize_feature_flags_t<
      Socket<feature::Item<type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                           feature::InOut<void>>>,
      Flags...>;

  template <class... Flags>
  class AsyncSocket;

  template <class... Flags>
  using AsyncSocketCompose = feature::organize_feature_flags_t<
      AsyncSocket<feature::Item<type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                                feature::InOut<void>>,
                  feature::Dyn, feature::Own>,
      Flags...>;

  template <class Traits>
  class Socket<feature::In<Traits>>
      : public RawDevice<feature::In<typename Traits::device_traits_type>> {
  private:
    using Base = RawDevice<feature::In<typename Traits::device_traits_type>>;

  public:
    using socket_traits_type = Traits;
    using async_type = AsyncSocketCompose<feature::In<Traits>>;

    using Base::Base;
    using Base::operator=;
  };

  template <class Traits>
  class Socket<feature::Out<Traits>>
      : public RawDevice<feature::Out<typename Traits::device_traits_type>> {
  private:
    using Base = RawDevice<feature::Out<typename Traits::device_traits_type>>;

  public:
    using socket_traits_type = Traits;
    using async_type = AsyncSocketCompose<feature::Out<Traits>>;

    using Base::Base;
    using Base::operator=;
  };

  template <class Trait>
  class Socket<feature::InOut<Trait>>
      : public RawDevice<feature::InOut<typename Trait::device_traits_type>> {
  private:
    using Base = RawDevice<feature::InOut<typename Trait::device_traits_type>>;

  public:
    using socket_traits_type = Trait;
    using async_type = AsyncSocketCompose<feature::InOut<socket_traits_type>>;

    template <template <class> class InOut = feature::InOut>
    using rebind = Socket<InOut<Trait>>;

    using Base::Base;
    using Base::operator=;

    Socket(Socket &&) = default;
    Socket &operator=(Socket &&) = default;
  };

  template <class Traits, class T, class U>
  class AsyncSocket<feature::In<Traits>, T, U>
      : public AsyncRawDevice<feature::In<typename Traits::device_traits_type>, T, U> {
    using Base = AsyncRawDevice<feature::In<typename Traits::device_traits_type>, T, U>;

  public:
    using typename Base::value_type;
    using socket_traits_type = Traits;
    using dynamic_type = AsyncSocket<feature::In<Traits>, feature::Dyn, U>;

    using Base::Base;
    using Base::operator=;

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> read(std::span<value_type> buf) {
      return imm_recv<Executor>(*this, buf);
    }

    Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }
  };

  template <class Traits, class T, class U>
  class AsyncSocket<feature::Out<Traits>, T, U>
      : public AsyncRawDevice<feature::Out<typename Traits::device_traits_type>, T, U> {
    using Base = AsyncRawDevice<feature::Out<typename Traits::device_traits_type>, T, U>;

  public:
    using typename Base::value_type;
    using socket_traits_type = Traits;
    using dynamic_type = AsyncSocket<feature::Out<Traits>, feature::Dyn, U>;

    using Base::Base;
    using Base::operator=;

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> write(std::span<const value_type> buf) {
      return imm_send<Executor>(*this, buf);
    }

    Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }
  };

  template <class Traits, class T, class U>
  class AsyncSocket<feature::InOut<Traits>, T, U>
      : public AsyncRawDevice<feature::InOut<typename Traits::device_traits_type>, T, U> {
    using Base = AsyncRawDevice<feature::InOut<typename Traits::device_traits_type>, T, U>;

  public:
    using typename Base::value_type;
    using socket_traits_type = Traits;
    using dynamic_type = AsyncSocket<feature::InOut<Traits>, feature::Dyn, U>;

    template <template <class> class InOut = feature::InOut>
    using rebind = AsyncSocket<InOut<Traits>, T, U>;

    using Base::Base;
    using Base::operator=;

    AsyncSocket(AsyncSocket &&) = default;
    AsyncSocket &operator=(AsyncSocket &&) = default;

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> read(std::span<value_type> buf) {
      return imm_recv<Executor>(*this, buf);
    }

    Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> write(std::span<const value_type> buf) {
      return imm_send<Executor>(*this, buf);
    }

    Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }
  };
}  // namespace impl_sock

/**
 * @brief determine the socket type
 *
 * @tparam Flags, can be feature::Tcp<feature::Ip<4>>, feature::Tcp<feature::Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using Socket = impl_sock::SocketCompose<feature::InOut<SocketTraits<Flags...>>>;

template <class Socket>
std::expected<Socket, std::errc> make_socket() {
  int fd = ::socket(Socket::socket_traits_type::family, Socket::socket_traits_type::type,
                    Socket::socket_traits_type::protocol);
  if (fd < 0) {
    return std::unexpected{std::errc(errno)};
  }
  return Socket{fd};
}
/**
 * @brief determine the socket type asynchronously
 *
 * @tparam Flags, can be feature::Tcp<feature::Ip<4>>, feature::Tcp<feature::Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using AsyncSocket = impl_sock::AsyncSocketCompose<feature::InOut<SocketTraits<Flags...>>>;
XSL_SYS_NET_NE
#endif
