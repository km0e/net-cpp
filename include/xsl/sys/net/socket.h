/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
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
  using SocketCompose
      = organize_feature_flags_t<Socket<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>,
                                 Flags...>;

  template <class... Flags>
  class AsyncSocket;

  template <class... Flags>
  using AsyncSocketCompose = organize_feature_flags_t<
      AsyncSocket<Item<is_same_pack, In<void>, Out<void>, InOut<void>>, Dyn, Own>, Flags...>;

  template <class Traits>
  class Socket<In<Traits>> : public RawDevice<In<typename Traits::device_traits_type>> {
  private:
    using Base = RawDevice<In<typename Traits::device_traits_type>>;

  public:
    using socket_traits_type = Traits;                  ///< indicate the socket traits
    using async_type = AsyncSocketCompose<In<Traits>>;  ///< indicate the async socket type

    using Base::Base;
    using Base::operator=;
  };

  template <class Traits>
  class Socket<Out<Traits>> : public RawDevice<Out<typename Traits::device_traits_type>> {
  private:
    using Base = RawDevice<Out<typename Traits::device_traits_type>>;

  public:
    using socket_traits_type = Traits;                   ///< indicate the socket traits
    using async_type = AsyncSocketCompose<Out<Traits>>;  ///< indicate the async socket type

    using Base::Base;
    using Base::operator=;
  };

  template <class Trait>
  class Socket<InOut<Trait>> : public RawDevice<InOut<typename Trait::device_traits_type>> {
  private:
    using Base = RawDevice<InOut<typename Trait::device_traits_type>>;

  public:
    using socket_traits_type = Trait;  ///< indicate the socket traits
    using async_type = AsyncSocketCompose<InOut<socket_traits_type>>;  ///< indicate the async
                                                                       ///< socket type

    template <template <class> class InOut = InOut>
    using rebind = Socket<InOut<Trait>>;  ///< rebind the socket type, for example, change the
                                          ///< socket type from InOut to In

    using Base::Base;
    using Base::operator=;

    Socket(Socket &&) = default;             ///< move constructor
    Socket &operator=(Socket &&) = default;  ///< move assignment
  };

  template <class Traits, class T, class U>
  class AsyncSocket<In<Traits>, T, U>
      : public AsyncRawDevice<In<typename Traits::device_traits_type>, T, U> {
    using Base = AsyncRawDevice<In<typename Traits::device_traits_type>, T, U>;

  public:
    using typename Base::value_type;
    using socket_traits_type = Traits;
    using dynamic_type = AsyncSocket<In<Traits>, Dyn, U>;

    using Base::Base;
    using Base::operator=;

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> read(std::span<value_type> buf) {
      return imm_recv<Executor>(*this, buf);
    }

    Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }
  };

  template <class Traits, class T, class U>
  class AsyncSocket<Out<Traits>, T, U>
      : public AsyncRawDevice<Out<typename Traits::device_traits_type>, T, U> {
    using Base = AsyncRawDevice<Out<typename Traits::device_traits_type>, T, U>;

  public:
    using typename Base::value_type;
    using socket_traits_type = Traits;
    using dynamic_type = AsyncSocket<Out<Traits>, Dyn, U>;

    using Base::Base;
    using Base::operator=;

    /**
     * @brief write the data to the socket
     *
     * @tparam Executor the executor type
     * @param buf the data buffer
     * @return Task<Result, Executor>
     */
    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> write(std::span<const value_type> buf) {
      return imm_send<Executor>(*this, buf);
    }

    /**
     * @brief write the data to the socket
     *
     * @param buf the data buffer
     * @return Task<Result>
     */
    Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }
  };

  template <class Traits, class T, class U>
  class AsyncSocket<InOut<Traits>, T, U>
      : public AsyncRawDevice<InOut<typename Traits::device_traits_type>, T, U> {
    using Base = AsyncRawDevice<InOut<typename Traits::device_traits_type>, T, U>;

  public:
    using typename Base::value_type;
    using socket_traits_type = Traits;
    using dynamic_type = AsyncSocket<InOut<Traits>, Dyn, U>;

    template <template <class> class InOut = InOut>
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
 * @tparam Flags, can be Tcp<Ip<4>>, Tcp<Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using Socket = impl_sock::SocketCompose<InOut<SocketTraits<Flags...>>>;

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
 * @tparam Flags, can be Tcp<Ip<4>>, Tcp<Ip<6>>, tag::TcpIpv4
 * ...
 */
template <class... Flags>
using AsyncSocket = impl_sock::AsyncSocketCompose<InOut<SocketTraits<Flags...>>>;
XSL_SYS_NET_NE
#endif
