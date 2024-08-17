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
#  include "xsl/ai.h"
#  include "xsl/feature.h"
#  include "xsl/sys/io/dev.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/io.h"

#  include <cassert>
#  include <memory>
#  include <tuple>

XSL_SYS_NET_NB
namespace impl_dev {
  template <class... Flags>
  class Device;

  template <class... Flags>
  using DeviceCompose = feature::organize_feature_flags_t<
      impl_dev::Device<feature::Item<type_traits::is_same_pack, feature::In<void>,
                                     feature::Out<void>, feature::InOut<void>>>,
      Flags...>;

  template <class... Flags>
  class AsyncDevice;

  template <class... Flags>
  using AsyncDeviceCompose = feature::organize_feature_flags_t<
      impl_dev::AsyncDevice<feature::Item<type_traits::is_same_pack, feature::In<void>,
                                          feature::Out<void>, feature::InOut<void>>,
                            feature::Dyn, feature::Own>,
      Flags...>;

  template <class Tag>
  class Device<feature::In<Tag>> : public io::RawDeviceOwner {
  private:
    using Base = io::RawDeviceOwner;

  public:
    using Base::Base;
    using tag_type = Tag;
    using device_features_type = feature::In<tag_type>;
    using device_traits_type = ai::DeviceTraits<tag_type>;
    using socket_traits_type = SocketTraits<tag_type>;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::In<T>>, aka AsyncDevice<feature::In<T>>
     */
    inline AsyncDeviceCompose<feature::In<tag_type>> async(this Device self,
                                                           Poller &poller) noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(self.raw(), IOM_EVENTS::IN | IOM_EVENTS::ET,
                 PollCallback<typename tag_type::poll_traits, IOM_EVENTS::IN>{sem});
      return {std::move(sem), std::move(self)};
    }
  };

  // static_assert(std::is_same_v<Device<feature::In<int>>, DeviceCompose<feature::In<int>>>,
  //               "Device<feature::In, feature::placeholder> is not DeviceCompose<feature::In>");

  template <class Tag>
  class Device<feature::Out<Tag>> : public io::RawDeviceOwner {
  private:
    using Base = io::RawDeviceOwner;

  public:
    using Base::Base;
    using tag_type = Tag;
    using device_features_type = feature::Out<tag_type>;
    using device_traits_type = ai::DeviceTraits<tag_type>;
    using socket_traits_type = SocketTraits<tag_type>;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::Out<T>>, aka AsyncDevice<feature::Out<T>>
     */
    inline AsyncDeviceCompose<feature::Out<tag_type>> async(this Device self,
                                                            Poller &poller) noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(self.raw(), IOM_EVENTS::OUT | IOM_EVENTS::ET,
                 PollCallback<typename socket_traits_type::poll_traits, IOM_EVENTS::OUT>{sem});
      return {std::move(sem), std::move(self)};
    }
  };

  // static_assert(std::is_same_v<Device<feature::Out<int>>, DeviceCompose<feature::Out<int>>>,
  //               "Device<feature::placeholder, feature::Out> is not DeviceCompose<feature::Out>");

  template <class Tag>
  class Device<feature::InOut<Tag>> : public io::RawDeviceOwner {
  private:
    using Base = io::RawDeviceOwner;

  public:
    using Base::Base;
    using tag_type = Tag;
    using device_features_type = feature::InOut<tag_type>;
    using device_traits_type = ai::DeviceTraits<tag_type>;
    using socket_traits_type = SocketTraits<tag_type>;

    std::tuple<DeviceCompose<feature::In<tag_type>>, DeviceCompose<feature::Out<tag_type>>> split(
        this Device self) noexcept {
      auto [r_dev, w_dev] = std::move(self).Base::split();
      auto r = DeviceCompose<feature::In<tag_type>>(std::move(r_dev));
      auto w = DeviceCompose<feature::Out<tag_type>>(std::move(w_dev));
      return {std::move(r), std::move(w)};
    }
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::InOut<T>>, aka AsyncDevice<feature::InOut<T>>
     */
    inline AsyncDeviceCompose<feature::InOut<tag_type>> async(this Device self,
                                                              Poller &poller) noexcept {
      auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
      auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(
          self.raw(), IOM_EVENTS::IN | IOM_EVENTS::OUT | IOM_EVENTS::ET,
          PollCallback<typename socket_traits_type::poll_traits, IOM_EVENTS::IN, IOM_EVENTS::OUT>{
              read_sem, write_sem});
      return {std::move(read_sem), std::move(write_sem), std::move(self)};
    }
  };

  // static_assert(
  //     std::is_same_v<Device<feature::InOut<int>>, DeviceCompose<feature::InOut<int>>>,
  //     "Device<feature::In, feature::Out> is not DeviceCompose<feature::In, feature::Out>");

  // static_assert(
  //     SocketLike<DeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
  //     "DeviceCompose<feature::InOut> is not SocketLike");

  template <class Tag, class T, class U>
  class AsyncDevice<feature::In<Tag>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::In<Tag>, U>, feature::placeholder> {
    template <class...>
    friend class AsyncDevice;
    using device_traits_type = ai::DeviceTraits<Tag>;
    using dynamic_device_type = ai::AsyncDevice<feature::In<Tag>, U>;

  public:
    using tag_type = Tag;
    using device_features_type = feature::In<tag_type>;
    using socket_traits_type = SocketTraits<tag_type>;
    using value_type = typename device_traits_type::value_type;
    using sem_type = coro::CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<Device<device_features_type>, Args...>
    AsyncDevice(Sem &&sem, Args &&...args) noexcept
        : _sem(std::forward<Sem>(sem)), _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncDevice(AsyncDevice<feature::In<Tag>, Flags...> &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    sem_type &sem() { return *_sem; }

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> read(std::span<value_type> buf) {
      return immediate_recv<Executor>(*this, buf);
    }

    Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }

    AsyncDeviceCompose<device_features_type, feature::Dyn, U> to_dyn(
        this AsyncDevice self) noexcept {
      return {std::move(self)};
    }

    std::unique_ptr<dynamic_device_type> to_unique_dyn(this AsyncDevice self) noexcept {
      return std::make_unique<AsyncDeviceCompose<device_features_type, feature::Dyn, U>>(
          std::move(self));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    Device<device_features_type> _dev;
  };

  // static_assert(
  //     std::is_same_v<AsyncDevice<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                feature::placeholder, feature::placeholder>,
  //                    AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
  //     "AsyncDevice<feature::In, feature::placeholder> is not AsyncDeviceCompose<feature::In>");

  // static_assert(
  //     std::is_same_v<AsyncDevice<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                feature::Dyn, feature::placeholder>,
  //                    AsyncDeviceCompose<feature::In<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                       feature::Dyn>>,
  //     "AsyncDevice<feature::In, feature::placeholder, feature::Dyn> is not "
  //     "AsyncDeviceCompose<feature::In, feature::Dyn>");

  // static_assert(std::is_base_of_v<ai::AsyncDevice<feature::In<tag::TcpIp>>,
  //                                 AsyncDeviceCompose<feature::In<tag::TcpIp>, feature::Dyn>>,
  //               "AsyncDeviceCompose<feature::In, feature::Dyn> is not derived from "
  //               "ai::Device<feature::In>");

  template <class Tag, class T, class U>
  class AsyncDevice<feature::Out<Tag>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::Out<Tag>, U>, feature::placeholder> {
    template <class...>
    friend class AsyncDevice;

    using device_traits_type = ai::DeviceTraits<Tag>;
    using dynamic_device_type = ai::AsyncDevice<feature::Out<Tag>, U>;

  public:
    using tag_type = Tag;
    using device_features_type = feature::Out<tag_type>;
    using socket_traits_type = SocketTraits<tag_type>;
    using value_type = typename device_traits_type::value_type;
    using sem_type = coro::CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<Device<device_features_type>, Args...>
    AsyncDevice(Sem &&sem, Args &&...args) noexcept
        : _sem(std::forward<Sem>(sem)), _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncDevice(AsyncDevice<feature::Out<Tag>, Flags...> &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept = default;

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept = default;

    ~AsyncDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    sem_type &sem() { return *_sem; }

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> write(std::span<const value_type> buf) {
      return immediate_send<Executor>(*this, buf);
    }

    Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }

    AsyncDeviceCompose<device_features_type, feature::Dyn, U> to_dyn(
        this AsyncDevice self) noexcept {
      return {std::move(self)};
    }

    std::unique_ptr<dynamic_device_type> to_unique_dyn(this AsyncDevice self) noexcept {
      return std::make_unique<AsyncDeviceCompose<device_features_type, feature::Dyn, U>>(
          std::move(self));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    Device<device_features_type> _dev;
  };

  // static_assert(
  //     std::is_same_v<AsyncDevice<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                feature::placeholder, feature::placeholder>,
  //                    AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
  //     "AsyncDevice<feature::placeholder, feature::Out> is not AsyncDeviceCompose<feature::Out>");

  // static_assert(
  //     std::is_same_v<AsyncDevice<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                feature::Dyn, feature::placeholder>,
  //                    AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                       feature::Dyn>>,
  //     "AsyncDevice<feature::placeholder, feature::Out, feature::Dyn> is not "
  //     "AsyncDeviceCompose<feature::Out, feature::Dyn>");

  // static_assert(
  //     std::is_base_of_v<ai::AsyncDevice<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>>,
  //                       AsyncDeviceCompose<feature::Out<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                          feature::Dyn>>,
  //     "AsyncDeviceCompose<feature::Out, feature::Dyn> is not derived from "
  //     "ai::Device<feature::Out>");
  template <class Tag, class T, class U>
  class AsyncDevice<feature::InOut<Tag>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::InOut<Tag>, U>, feature::placeholder> {
    template <class...>
    friend class AsyncDevice;

    using device_traits_type = ai::DeviceTraits<Tag>;
    using dynamic_device_type = ai::AsyncDevice<feature::InOut<Tag>, U>;

  public:
    using tag_type = Tag;
    using device_features_type = feature::InOut<tag_type>;
    using socket_traits_type = SocketTraits<tag_type>;
    using value_type = typename device_traits_type::value_type;
    using sem_type = coro::CountingSemaphore<1>;
    using inner_type = DeviceCompose<device_features_type>;

    template <template <class> class InOut>
    using rebind = AsyncDevice<InOut<tag_type>, T, U>;

    using in_dev_type = rebind<feature::In>;

    using out_dev_type = rebind<feature::Out>;

    AsyncDevice() noexcept : _read_sem(nullptr), _write_sem(nullptr), _dev() {}
    template <class ReadSem, class WriteSem, class... Args>
    AsyncDevice(ReadSem &&read_sem, WriteSem &&write_sem, Args &&...args) noexcept
        : _read_sem(std::forward<ReadSem>(read_sem)),
          _write_sem(std::forward<WriteSem>(write_sem)),
          _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncDevice(AsyncDevice<feature::InOut<Tag>, Flags...> &&rhs) noexcept
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

    Task<Result> read(std::span<byte> buf) {
      return immediate_recv<coro::ExecutorBase>(*this, buf);
    }

    Task<Result> write(std::span<const byte> buf) {
      return immediate_send<coro::ExecutorBase>(*this, buf);
    }

    AsyncDeviceCompose<device_features_type, feature::Dyn, U> to_dyn(
        this AsyncDevice self) noexcept {
      return {std::move(self)};
    }

    std::unique_ptr<dynamic_device_type> to_unique_dyn(this AsyncDevice self) noexcept {
      return std::make_unique<AsyncDeviceCompose<device_features_type, feature::Dyn, U>>(
          std::move(self));
    }

    /**
     * @brief split the device into two devices
     *
     * @return std::tuple<AsyncDevice<feature::In<socket_traits_type>, T, U>,
     * AsyncDevice<feature::Out<socket_traits_type>, T, U>>
     */
    std::tuple<AsyncDevice<feature::In<tag_type>, T, U>, AsyncDevice<feature::Out<tag_type>, T, U>>
    split(this AsyncDevice self) noexcept {
      auto [r, w] = std::move(self._dev).split();
      using In = AsyncDevice<feature::In<tag_type>, T, U>;
      using Out = AsyncDevice<feature::Out<tag_type>, T, U>;
      auto _in = In{std::move(self._read_sem), std::move(r)};
      auto _out = Out{std::move(self._write_sem), std::move(w)};
      return {std::move(_in), std::move(_out)};
    }

  protected:
    std::shared_ptr<sem_type> _read_sem, _write_sem;
    DeviceCompose<device_features_type> _dev;
  };

  // static_assert(std::is_same_v<
  //                   AsyncDevice<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                               feature::placeholder, feature::placeholder>,
  //                   AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>>>,
  //               "AsyncDevice<feature::In, feature::Out> is not AsyncDeviceCompose<feature::In, "
  //               "feature::Out>");

  // static_assert(
  //     std::is_same_v<AsyncDevice<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                feature::Dyn, feature::placeholder>,
  //                    AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                       feature::Dyn>>,
  //     "AsyncDevice<feature::In, feature::Out, feature::Dyn> is not "
  //     "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn>");

  // static_assert(std::is_base_of_v<
  //                   ai::AsyncDevice<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>>,
  //                   AsyncDeviceCompose<feature::InOut<SocketTraits<feature::Tcp<feature::Ip<4>>>>,
  //                                      feature::Dyn>>,
  //               "AsyncDeviceCompose<feature::In, feature::Out, feature::Dyn> is not derived from
  //               " "ai::Device<>");

}  // namespace impl_dev

template <class... Flags>
using Device = impl_dev::DeviceCompose<Flags...>;

template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;

XSL_SYS_NET_NE
#endif
