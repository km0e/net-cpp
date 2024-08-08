/**
@file dev.h
@author Haixin Pang (kmdr.error@gmail.com)
@brief
@version 0.1
@date 2024-08-06

@copyright Copyright (c) 2024

 */

#pragma once

#ifndef XSL_SYS_NET_DEV
#  define XSL_SYS_NET_DEV
#  include "xsl/ai/dev.h"
#  include "xsl/feature.h"
#  include "xsl/sys/io/dev.h"
#  include "xsl/sys/net/def.h"

#  include <cassert>
#  include <cstddef>
#  include <tuple>
SYS_NET_NB
namespace impl_dev {
  using ai::Result;

  template <class... Flags>
  class AsyncDevice;

  template <class... Flags>
  using AsyncDeviceCompose = feature::origanize_feature_flags_t<
      impl_dev::AsyncDevice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                          feature::Out<void>, feature::InOut<void>>,
                            feature::Dyn, feature::Own>,
      Flags...>;

  template <class T, class U>
  class AsyncDevice<feature::In<std::byte>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::In<std::byte>, U>,
                                  feature::placeholder> {
  public:
    using value_type = std::byte;

    template <class... Args>
    AsyncDevice(Args &&...args) noexcept : _dev(std::forward<Args>(args)...) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    decltype(auto) sem() { return _dev.sem(); }

    template <class Executor = coro::ExecutorBase>
    coro::Task<Result, Executor> read(std::span<value_type> buf) {
      ssize_t n;
      size_t offset = 0;
      while (true) {
        n = ::recv(this->_dev.raw(), buf.data() + offset, buf.size() - offset, 0);
        WARN("{} recv n: {}", this->_dev.raw(), n);
        if (n == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (offset != 0) {
              WARN("recv over");
              break;
            }
            WARN("no data");
            if (!co_await this->_dev.sem()) {
              co_return Result(offset, {std::errc::not_connected});
            }
            continue;
          } else {
            WARN("Failed to recv data, err : {}", strerror(errno));
            // TODO: handle recv error
            co_return Result(offset, {std::errc(errno)});
          }
        } else if (n == 0) {
          WARN("recv eof");
          if (offset == 0) {
            co_return Result(offset, {std::errc::no_message});
          }
          co_return Result(offset, std::nullopt);
        }
        WARN("recv {} bytes", n);
        offset += n;
      };
      LOG5("end recv string");
      co_return std::make_tuple(offset, std::nullopt);
    }
    coro::Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }

    AsyncDeviceCompose<feature::In<value_type>, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(_dev)};
    }

  protected:
    io::AsyncDevice<feature::In<value_type>> _dev;
  };

  static_assert(
      std::is_same_v<
          AsyncDevice<feature::In<std::byte>, feature::placeholder, feature::placeholder>,
          AsyncDeviceCompose<feature::In<std::byte>>>,
      "AsyncDevice<feature::In, feature::placeholder> is not AsyncDeviceCompose<feature::In>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::In<std::byte>, feature::Dyn, feature::placeholder>,
                     AsyncDeviceCompose<feature::In<std::byte>, feature::Dyn>>,
      "AsyncDevice<feature::In, feature::placeholder, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::In, feature::Dyn>");

  static_assert(std::is_base_of_v<ai::AsyncDevice<feature::In<std::byte>>,
                                  AsyncDeviceCompose<feature::In<std::byte>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::In, feature::Dyn> is not derived from "
                "ai::Device<feature::In>");

  template <class T, class U>
  class AsyncDevice<feature::Out<std::byte>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::Out<std::byte>, U>,
                                  feature::placeholder> {
  public:
    using value_type = std::byte;

    template <class... Args>
    AsyncDevice(Args &&...args) noexcept : _dev(std::forward<Args>(args)...) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    decltype(auto) sem() { return _dev.sem(); }

    template <class Executor = coro::ExecutorBase>
    coro::Task<Result, Executor> write(std::span<const value_type> buf) {
      using Result = ai::Result;
      while (true) {
        ssize_t n = ::send(this->_dev.raw(), buf.data(), buf.size(), 0);
        if (static_cast<size_t>(n) == buf.size()) {
          WARN("send {} bytes", n);
          co_return {n, std::nullopt};
        }
        if (n > 0) {
          WARN("send {} bytes", n);
          buf = buf.subspan(n);
          continue;
        }
        if (n == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
          co_return Result{0, {std::errc(errno)}};
        }
        if (!co_await this->_dev.sem()) {
          co_return Result{0, {std::errc::not_connected}};
        }
      }
    }

    coro::Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }

    AsyncDeviceCompose<feature::Out<value_type>, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(_dev)};
    }

  protected:
    io::AsyncDevice<feature::Out<value_type>> _dev;
  };

  static_assert(
      std::is_same_v<
          AsyncDevice<feature::Out<std::byte>, feature::placeholder, feature::placeholder>,
          AsyncDeviceCompose<feature::Out<std::byte>>>,
      "AsyncDevice<feature::placeholder, feature::Out> is not AsyncDeviceCompose<feature::Out>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::Out<std::byte>, feature::Dyn, feature::placeholder>,
                     AsyncDeviceCompose<feature::Out<std::byte>, feature::Dyn>>,
      "AsyncDevice<feature::placeholder, feature::Out, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::Out, feature::Dyn>");

  static_assert(std::is_base_of_v<ai::AsyncDevice<feature::Out<std::byte>>,
                                  AsyncDeviceCompose<feature::Out<std::byte>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::Out, feature::Dyn> is not derived from "
                "ai::Device<feature::Out>");
  template <class T, class U>
  class AsyncDevice<feature::InOut<std::byte>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::InOut<std::byte>, U>,
                                  feature::placeholder> {
  public:
    template <class... Args>
    AsyncDevice(Args &&...args) noexcept : _dev(std::forward<Args>(args)...) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    decltype(auto) inner() { return _dev.inner(); }

    decltype(auto) read_sem() { return _dev.read_sem(); }

    coro::Task<Result> read(std::span<std::byte> buf) { assert(false && "Not implemented"); }

    coro::Task<Result> write(std::span<const std::byte> buf) { assert(false && "Not implemented"); }

    AsyncDeviceCompose<feature::InOut<std::byte>, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(_dev)};
    }

    std::tuple<AsyncDevice<feature::In<std::byte>, T, U>,
               AsyncDevice<feature::Out<std::byte>, T, U>>
    split() && noexcept {
      auto [r, w] = std::move(_dev).split();
      return {AsyncDevice<feature::In<std::byte>, T, U>{std::move(r)},
              AsyncDevice<feature::Out<std::byte>, T, U>{std::move(w)}};
    }

  protected:
    io::AsyncDevice<feature::InOut<std::byte>> _dev;
  };

  static_assert(std::is_same_v<AsyncDevice<feature::InOut<std::byte>, feature::placeholder,
                                           feature::placeholder>,
                               AsyncDeviceCompose<feature::InOut<std::byte>>>,
                "AsyncDevice<feature::In, feature::Out> is not AsyncDeviceCompose<feature::In, "
                "feature::Out>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::InOut<std::byte>, feature::Dyn, feature::placeholder>,
                     AsyncDeviceCompose<feature::InOut<std::byte>, feature::Dyn>>,
      "AsyncDevice<feature::In, feature::Out, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn>");

  static_assert(std::is_base_of_v<ai::AsyncDevice<feature::InOut<std::byte>>,
                                  AsyncDeviceCompose<feature::InOut<std::byte>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn> is not derived from "
                "ai::Device<>");

}  // namespace impl_dev

template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;

SYS_NET_NE
#endif
