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
  class Device;

  template <class... Flags>
  using DeviceCompose = feature::organize_feature_flags_t<
      impl_dev::Device<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                     feature::Out<void>, feature::InOut<void>>>,
      Flags...>;

  template <class... Flags>
  class AsyncDevice;

  template <class... Flags>
  using AsyncDeviceCompose = feature::organize_feature_flags_t<
      impl_dev::AsyncDevice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                          feature::Out<void>, feature::InOut<void>>,
                            feature::Dyn, feature::Own>,
      Flags...>;

  template <class Traits>
  class Device<feature::In<Traits>> : public io::NativeDeviceOwner {
  private:
    using Base = io::NativeDeviceOwner;

  public:
    using Base::Base;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::In<T>>, aka AsyncDevice<feature::In<T>>
     */
    inline AsyncDeviceCompose<feature::In<Traits>> async(sync::Poller &poller) && noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(this->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
                 sync::PollCallback<typename Traits::poll_traits, sync::IOM_EVENTS::IN>{sem});
      return {std::move(sem), std::move(*this)};
    }
  };

  static_assert(std::is_same_v<Device<feature::In<int>>, DeviceCompose<feature::In<int>>>,
                "Device<feature::In, feature::placeholder> is not DeviceCompose<feature::In>");

  template <class Traits>
  class Device<feature::Out<Traits>> : public io::NativeDeviceOwner {
  private:
    using Base = io::NativeDeviceOwner;

  public:
    using Base::Base;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::Out<T>>, aka AsyncDevice<feature::Out<T>>
     */
    inline AsyncDeviceCompose<feature::Out<Traits>> async(sync::Poller &poller) && noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(this->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                 sync::PollCallback<typename Traits::poll_traits, sync::IOM_EVENTS::OUT>{sem});
      return {std::move(sem), std::move(*this)};
    }
  };

  static_assert(std::is_same_v<Device<feature::Out<int>>, DeviceCompose<feature::Out<int>>>,
                "Device<feature::placeholder, feature::Out> is not DeviceCompose<feature::Out>");

  template <class Traits>
  class Device<feature::InOut<Traits>> : public io::NativeDeviceOwner {
  private:
    using Base = io::NativeDeviceOwner;

  public:
    using Base::Base;
    using traits_type = Traits;

    std::tuple<DeviceCompose<feature::In<Traits>>, DeviceCompose<feature::Out<Traits>>> split()
        && noexcept {
      auto r = DeviceCompose<feature::In<Traits>>{this->_dev};
      auto w = DeviceCompose<feature::Out<Traits>>{std::move(this->_dev)};
      // TODO: need suitable error handling
      return {std::move(r), std::move(w)};
    }
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::InOut<T>>, aka AsyncDevice<feature::InOut<T>>
     */
    inline AsyncDeviceCompose<feature::InOut<Traits>> async(sync::Poller &poller) && noexcept {
      auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
      auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(this->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                 sync::PollCallback<typename Traits::poll_traits, sync::IOM_EVENTS::IN,
                                    sync::IOM_EVENTS::OUT>{read_sem, write_sem});
      return {std::move(read_sem), std::move(write_sem), std::move(*this)};
    }
  };

  static_assert(
      std::is_same_v<Device<feature::InOut<int>>, DeviceCompose<feature::InOut<int>>>,
      "Device<feature::In, feature::Out> is not DeviceCompose<feature::In, feature::Out>");

  template <class Traits, class T, class U>
  class AsyncDevice<feature::In<Traits>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::In<std::byte>, U>,
                                  feature::placeholder> {
    template <class...>
    friend class AsyncDevice;

  public:
    using value_type = std::byte;
    using traits_type = Traits;
    using sem_type = coro::CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<Device<feature::In<traits_type>>, Args...>
    AsyncDevice(Sem &&sem, Args &&...args) noexcept
        : _sem(std::forward<Sem>(sem)), _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncDevice(AsyncDevice<feature::In<Traits>, Flags...> &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    sem_type &sem() { return *_sem; }

    template <class Executor = coro::ExecutorBase>
    coro::Task<Result, Executor> read(std::span<value_type> buf) {
      ssize_t n;
      size_t offset = 0;
      while (true) {
        n = ::recv(this->_dev.raw(), buf.data() + offset, buf.size() - offset, 0);
        DEBUG("{} recv n: {}", this->_dev.raw(), n);
        if (n == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (offset != 0) {
              WARN("recv over");
              break;
            }
            WARN("no data");
            if (!co_await *this->_sem) {
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
        DEBUG("recv {} bytes", n);
        offset += n;
      };
      LOG5("end recv string");
      co_return std::make_tuple(offset, std::nullopt);
    }
    coro::Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }

    AsyncDeviceCompose<feature::In<traits_type>, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(*this)};
    }

    decltype(auto) to_unique_dyn() && noexcept {
      return std::make_unique<AsyncDeviceCompose<feature::In<traits_type>, feature::Dyn, U>>(
          std::move(*this));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    Device<feature::In<traits_type>> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::In<SocketTraits<feature::Tcp>>, feature::placeholder,
                                 feature::placeholder>,
                     AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp>>>>,
      "AsyncDevice<feature::In, feature::placeholder> is not AsyncDeviceCompose<feature::In>");

  static_assert(
      std::is_same_v<
          AsyncDevice<feature::In<SocketTraits<feature::Tcp>>, feature::Dyn, feature::placeholder>,
          AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp>>, feature::Dyn>>,
      "AsyncDevice<feature::In, feature::placeholder, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::In, feature::Dyn>");

  static_assert(
      std::is_base_of_v<ai::AsyncDevice<feature::In<std::byte>>,
                        AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp>>, feature::Dyn>>,
      "AsyncDeviceCompose<feature::In, feature::Dyn> is not derived from "
      "ai::Device<feature::In>");

  template <class Traits, class T, class U>
  class AsyncDevice<feature::Out<Traits>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::Out<std::byte>, U>,
                                  feature::placeholder> {
    template <class...>
    friend class AsyncDevice;

  public:
    using value_type = std::byte;
    using traits_type = Traits;
    using sem_type = coro::CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<Device<feature::Out<traits_type>>, Args...>
    AsyncDevice(Sem &&sem, Args &&...args) noexcept
        : _sem(std::forward<Sem>(sem)), _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncDevice(AsyncDevice<feature::Out<Traits>, Flags...> &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    sem_type &sem() { return *_sem; }

    template <class Executor = coro::ExecutorBase>
    coro::Task<Result, Executor> write(std::span<const value_type> buf) {
      using Result = ai::Result;
      while (true) {
        ssize_t n = ::send(this->_dev.raw(), buf.data(), buf.size(), 0);
        if (static_cast<size_t>(n) == buf.size()) {
          DEBUG("{} send {} bytes", this->_dev.raw(), n);
          co_return {n, std::nullopt};
        }
        if (n > 0) {
          DEBUG("{} send {} bytes", this->_dev.raw(), n);
          buf = buf.subspan(n);
          continue;
        }
        if (n == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
          co_return Result{0, {std::errc(errno)}};
        }
        if (!co_await *this->_sem) {
          co_return Result{0, {std::errc::not_connected}};
        }
      }
    }

    coro::Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }

    AsyncDeviceCompose<feature::Out<traits_type>, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(*this)};
    }

    decltype(auto) to_unique_dyn() && noexcept {
      return std::make_unique<AsyncDeviceCompose<feature::Out<traits_type>, feature::Dyn, U>>(
          std::move(*this));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    Device<feature::Out<traits_type>> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::Out<SocketTraits<feature::Tcp>>, feature::placeholder,
                                 feature::placeholder>,
                     AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp>>>>,
      "AsyncDevice<feature::placeholder, feature::Out> is not AsyncDeviceCompose<feature::Out>");

  static_assert(
      std::is_same_v<
          AsyncDevice<feature::Out<SocketTraits<feature::Tcp>>, feature::Dyn, feature::placeholder>,
          AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp>>, feature::Dyn>>,
      "AsyncDevice<feature::placeholder, feature::Out, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::Out, feature::Dyn>");

  static_assert(
      std::is_base_of_v<ai::AsyncDevice<feature::Out<std::byte>>,
                        AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp>>, feature::Dyn>>,
      "AsyncDeviceCompose<feature::Out, feature::Dyn> is not derived from "
      "ai::Device<feature::Out>");
  template <class Traits, class T, class U>
  class AsyncDevice<feature::InOut<Traits>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::InOut<std::byte>, U>,
                                  feature::placeholder> {
    template <class...>
    friend class AsyncDevice;

  public:
    using value_type = std::byte;
    using traits_type = Traits;
    using sem_type = coro::CountingSemaphore<1>;
    using inner_type = DeviceCompose<feature::InOut<traits_type>>;

    template <template <class> class InOut>
    using rebind_type = AsyncDevice<InOut<traits_type>, T, U>;

    template <class ReadSem, class WriteSem, class... Args>
    AsyncDevice(ReadSem &&read_sem, WriteSem &&write_sem, Args &&...args) noexcept
        : _read_sem(std::forward<ReadSem>(read_sem)),
          _write_sem(std::forward<WriteSem>(write_sem)),
          _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncDevice(AsyncDevice<feature::InOut<Traits>, Flags...> &&rhs) noexcept
        : _read_sem(std::move(rhs._read_sem)),
          _write_sem(std::move(rhs._write_sem)),
          _dev(std::move(rhs._dev)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    inner_type &inner() { return _dev; }

    sem_type &read_sem() { return *_read_sem; }

    sem_type &write_sem() { return *_write_sem; }

    coro::Task<Result> read(std::span<std::byte> buf) { assert(false && "Not implemented"); }

    coro::Task<Result> write(std::span<const std::byte> buf) { assert(false && "Not implemented"); }

    AsyncDeviceCompose<feature::InOut<traits_type>, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(*this)};
    }

    decltype(auto) to_unique_dyn() && noexcept {
      return std::make_unique<AsyncDeviceCompose<feature::InOut<traits_type>, feature::Dyn, U>>(
          std::move(*this));
    }

    std::tuple<AsyncDevice<feature::In<traits_type>, T, U>,
               AsyncDevice<feature::Out<traits_type>, T, U>>
    split() && noexcept {
      auto [r, w] = std::move(_dev).split();
      using In = AsyncDevice<feature::In<traits_type>, T, U>;
      using Out = AsyncDevice<feature::Out<traits_type>, T, U>;
      auto _in = In{std::move(_read_sem), std::move(r)};
      auto _out = Out{std::move(_write_sem), std::move(w)};
      return {std::move(_in), std::move(_out)};
    }

  protected:
    std::shared_ptr<sem_type> _read_sem, _write_sem;
    DeviceCompose<feature::InOut<traits_type>> _dev;
  };

  static_assert(std::is_same_v<AsyncDevice<feature::InOut<SocketTraits<feature::Tcp>>,
                                           feature::placeholder, feature::placeholder>,
                               AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp>>>>,
                "AsyncDevice<feature::In, feature::Out> is not AsyncDeviceCompose<feature::In, "
                "feature::Out>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::InOut<SocketTraits<feature::Tcp>>, feature::Dyn,
                                 feature::placeholder>,
                     AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp>>, feature::Dyn>>,
      "AsyncDevice<feature::In, feature::Out, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn>");

  static_assert(std::is_base_of_v<
                    ai::AsyncDevice<feature::InOut<std::byte>>,
                    AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp>>, feature::Dyn>>,
                "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn> is not derived from "
                "ai::Device<>");

}  // namespace impl_dev

template <class... Flags>
using Device = impl_dev::DeviceCompose<Flags...>;

template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;

SYS_NET_NE
#endif
