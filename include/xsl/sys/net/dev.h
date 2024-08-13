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
#  include "xsl/sys/net/io.h"

#  include <cassert>
#  include <cstddef>
#  include <tuple>

XSL_SYS_NET_NB
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
    using device_traits_type = feature::In<Traits>;
    using socket_traits_type = Traits;
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
    using device_traits_type = feature::Out<Traits>;
    using socket_traits_type = Traits;
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
    using device_traits_type = feature::InOut<Traits>;
    using socket_traits_type = Traits;

    std::tuple<DeviceCompose<feature::In<Traits>>, DeviceCompose<feature::Out<Traits>>> split()
        && noexcept {
      auto [r_dev, w_dev] = std::move(*this).Base::split();
      auto r = DeviceCompose<feature::In<Traits>>(std::move(r_dev));
      auto w = DeviceCompose<feature::Out<Traits>>(std::move(w_dev));
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
    using device_traits_type = feature::In<Traits>;
    using socket_traits_type = Traits;
    using value_type = std::byte;
    using sem_type = coro::CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<Device<device_traits_type>, Args...>
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
      return immediate_recv<Executor>(*this, buf);
    }

    coro::Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }

    AsyncDeviceCompose<device_traits_type, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(*this)};
    }

    decltype(auto) to_unique_dyn() && noexcept {
      return std::make_unique<AsyncDeviceCompose<device_traits_type, feature::Dyn, U>>(
          std::move(*this));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    Device<device_traits_type> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                 feature::placeholder, feature::placeholder>,
                     AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
      "AsyncDevice<feature::In, feature::placeholder> is not AsyncDeviceCompose<feature::In>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                 feature::Dyn, feature::placeholder>,
                     AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                        feature::Dyn>>,
      "AsyncDevice<feature::In, feature::placeholder, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::In, feature::Dyn>");

  static_assert(
      std::is_base_of_v<ai::AsyncDevice<feature::In<std::byte>>,
                        AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                           feature::Dyn>>,
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
    using device_traits_type = feature::Out<Traits>;
    using socket_traits_type = Traits;
    using value_type = std::byte;
    using sem_type = coro::CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<Device<device_traits_type>, Args...>
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
      return immediate_send<Executor>(*this, buf);
    }

    coro::Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }

    AsyncDeviceCompose<device_traits_type, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(*this)};
    }

    decltype(auto) to_unique_dyn() && noexcept {
      return std::make_unique<AsyncDeviceCompose<device_traits_type, feature::Dyn, U>>(
          std::move(*this));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    Device<device_traits_type> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                 feature::placeholder, feature::placeholder>,
                     AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
      "AsyncDevice<feature::placeholder, feature::Out> is not AsyncDeviceCompose<feature::Out>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                 feature::Dyn, feature::placeholder>,
                     AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                        feature::Dyn>>,
      "AsyncDevice<feature::placeholder, feature::Out, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::Out, feature::Dyn>");

  static_assert(
      std::is_base_of_v<ai::AsyncDevice<feature::Out<std::byte>>,
                        AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                           feature::Dyn>>,
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
    using device_traits_type = feature::InOut<Traits>;
    using socket_traits_type = Traits;
    using value_type = std::byte;
    using sem_type = coro::CountingSemaphore<1>;
    using inner_type = DeviceCompose<device_traits_type>;

    template <template <class> class InOut>
    using rebind_type = AsyncDevice<InOut<socket_traits_type>, T, U>;

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

    coro::Task<Result> read(std::span<std::byte> buf) {
      return immediate_recv<coro::ExecutorBase>(*this, buf);
    }

    coro::Task<Result> write(std::span<const std::byte> buf) {
      return immediate_send<coro::ExecutorBase>(*this, buf);
    }

    AsyncDeviceCompose<device_traits_type, feature::Dyn, U> to_dyn() && noexcept {
      return {std::move(*this)};
    }

    decltype(auto) to_unique_dyn() && noexcept {
      return std::make_unique<AsyncDeviceCompose<device_traits_type, feature::Dyn, U>>(
          std::move(*this));
    }

    /**
     * @brief split the device into two devices
     *
     * @return std::tuple<AsyncDevice<feature::In<socket_traits_type>, T, U>,
     * AsyncDevice<feature::Out<socket_traits_type>, T, U>>
     */
    std::tuple<AsyncDevice<feature::In<socket_traits_type>, T, U>,
               AsyncDevice<feature::Out<socket_traits_type>, T, U>>
    split() && noexcept {
      auto [r, w] = std::move(_dev).split();
      using In = AsyncDevice<feature::In<socket_traits_type>, T, U>;
      using Out = AsyncDevice<feature::Out<socket_traits_type>, T, U>;
      auto _in = In{std::move(_read_sem), std::move(r)};
      auto _out = Out{std::move(_write_sem), std::move(w)};
      return {std::move(_in), std::move(_out)};
    }

  protected:
    std::shared_ptr<sem_type> _read_sem, _write_sem;
    DeviceCompose<device_traits_type> _dev;
  };

  static_assert(std::is_same_v<
                    AsyncDevice<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                feature::placeholder, feature::placeholder>,
                    AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
                "AsyncDevice<feature::In, feature::Out> is not AsyncDeviceCompose<feature::In, "
                "feature::Out>");

  static_assert(
      std::is_same_v<AsyncDevice<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                 feature::Dyn, feature::placeholder>,
                     AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                        feature::Dyn>>,
      "AsyncDevice<feature::In, feature::Out, feature::Dyn> is not "
      "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn>");

  static_assert(std::is_base_of_v<
                    ai::AsyncDevice<feature::InOut<std::byte>>,
                    AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
                                       feature::Dyn>>,
                "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn> is not derived from "
                "ai::Device<>");

}  // namespace impl_dev

template <class... Flags>
using Device = impl_dev::DeviceCompose<Flags...>;

template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;

XSL_SYS_NET_NE
#endif
