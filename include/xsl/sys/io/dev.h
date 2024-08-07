#pragma once
#ifndef XSL_SYS_IO_DEV
#  define XSL_SYS_IO_DEV
#  include "xsl/coro/semaphore.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/sync.h"
#  include "xsl/sys/io/def.h"
#  include "xsl/wheel/type_traits.h"

#  include <unistd.h>

#  include <tuple>
#  include <type_traits>
#  include <utility>
XSL_SYS_IO_NB
class NativeDevice {
public:
  NativeDevice(int fd) noexcept : _fd(fd) {}
  NativeDevice(NativeDevice &&rhs) noexcept : _fd(std::exchange(rhs._fd, -1)) {}
  NativeDevice &operator=(NativeDevice &&rhs) noexcept {
    _fd = std::exchange(rhs._fd, -1);
    return *this;
  }
  int raw() const noexcept { return _fd; }

  ~NativeDevice() noexcept {
    if (_fd == -1) {
      return;
    }
    LOG5("close fd: {}", _fd);
    close(_fd);
  }

protected:
  int _fd;
};

namespace impl_dev {
  template <class... Flags>
  class AsyncDevice;

  // using AsyncReadDevice = AsyncDevice<feature::In, feature::placeholder>;
  // using AsyncWriteDevice = AsyncDevice<feature::placeholder, feature::Out>;
  // using AsyncReadWriteDevice = AsyncDevice<feature::In, feature::Out>;

  template <class... Flags>
  using AsyncDeviceCompose = feature::origanize_feature_flags_t<
      impl_dev::AsyncDevice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                          feature::Out<void>, feature::InOut<void>>>,
      Flags...>;

  template <class... Flags>
  class Device;

  // using ReadDevice = Device<feature::In, feature::placeholder>;
  // using WriteDevice = Device<feature::placeholder, feature::Out>;
  // using ReadWriteDevice = Device<feature::In, feature::Out>;

  template <class... Flags>
  using DeviceCompose = feature::origanize_feature_flags_t<
      impl_dev::Device<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                     feature::Out<void>, feature::InOut<void>>>,
      Flags...>;

  template <>
  class Device<feature::placeholder> {
  public:
    explicit Device(NativeDevice &&dev) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))) {}
    Device(std::shared_ptr<NativeDevice> dev) noexcept : _dev(std::move(dev)) {}
    Device(Device &&rhs) noexcept : _dev(std::move(rhs._dev)) {}
    Device &operator=(Device &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }
    ~Device() noexcept { LOG6("Device dtor, use count: {}", _dev.use_count()); }
    int raw() const noexcept { return _dev->raw(); }

  protected:
    std::shared_ptr<NativeDevice> _dev;
  };

  template <class T>
  class Device<feature::In<T>> : public Device<feature::placeholder> {
  private:
    using Base = Device<feature::placeholder>;

  public:
    using Base::Base;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::In<T>>, aka AsyncDevice<feature::In<T>>
     */
    inline AsyncDeviceCompose<feature::In<T>> async(sync::Poller &poller) && noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
                 sync::PollCallback<sync::IOM_EVENTS::IN>{sem});
      return {std::move(sem), std::move(_dev)};
    }
  };

  static_assert(std::is_same_v<Device<feature::In<int>>, DeviceCompose<feature::In<int>>>,
                "Device<feature::In, feature::placeholder> is not DeviceCompose<feature::In>");

  template <class T>
  class Device<feature::Out<T>> : public Device<feature::placeholder> {
  private:
    using Base = Device<feature::placeholder>;

  public:
    using Base::Base;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::Out<T>>, aka AsyncDevice<feature::Out<T>>
     */
    inline AsyncDeviceCompose<feature::Out<T>> async(sync::Poller &poller) && noexcept {
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(_dev->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                 sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
      return {std::move(sem), std::move(_dev)};
    }
  };

  static_assert(std::is_same_v<Device<feature::Out<int>>, DeviceCompose<feature::Out<int>>>,
                "Device<feature::placeholder, feature::Out> is not DeviceCompose<feature::Out>");

  template <class T>
  class Device<feature::InOut<T>> : public Device<feature::placeholder> {
  private:
    using Base = Device<feature::placeholder>;

  public:
    using Base::Base;
    std::tuple<DeviceCompose<feature::In<T>>, DeviceCompose<feature::Out<T>>> split() && noexcept {
      auto r = DeviceCompose<feature::In<T>>{_dev};
      auto w = DeviceCompose<feature::Out<T>>{std::move(_dev)};
      return {std::move(r), std::move(w)};
    }
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::InOut<T>>, aka AsyncDevice<feature::InOut<T>>
     */
    inline AsyncDeviceCompose<feature::InOut<T>> async(sync::Poller &poller) && noexcept {
      auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
      auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(
          _dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
          sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{read_sem, write_sem});
      return {std::move(read_sem), std::move(write_sem), std::move(_dev)};
    }
  };

  static_assert(
      std::is_same_v<Device<feature::InOut<int>>, DeviceCompose<feature::InOut<int>>>,
      "Device<feature::In, feature::Out> is not DeviceCompose<feature::In, feature::Out>");

  template <class T>
  class AsyncDevice<feature::In<T>> {
  public:
    template <class... Args>
    AsyncDevice(std::shared_ptr<coro::CountingSemaphore<1>> sem, Args &&...args) noexcept
        : _sem(std::move(sem)), _dev(std::forward<Args>(args)...) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      _sem = std::move(rhs._sem);
      _dev = std::move(rhs._dev);
      return *this;
    }

    ~AsyncDevice() noexcept { LOG6("AsyncDevice dtor, use count: {}", _sem.use_count()); }

    int raw() const noexcept { return _dev.raw(); }

    DeviceCompose<feature::In<T>> &inner() noexcept { return _dev; }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
    DeviceCompose<feature::In<T>> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::In<int>>, AsyncDeviceCompose<feature::In<int>>>,
      "AsyncDevice<feature::In, feature::placeholder> is not AsyncDeviceCompose<feature::In>");

  template <class T>
  class AsyncDevice<feature::Out<T>> {
  public:
    template <class... Args>
    AsyncDevice(std::shared_ptr<coro::CountingSemaphore<1>> sem, Args &&...args) noexcept
        : _sem(std::move(sem)), _dev(std::forward<Args>(args)...) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      _sem = std::move(rhs._sem);
      _dev = std::move(rhs._dev);
      return *this;
    }

    ~AsyncDevice() noexcept { LOG6("AsyncDevice dtor, use count: {}", _sem.use_count()); }

    int raw() const noexcept { return _dev.raw(); }

    DeviceCompose<feature::Out<T>> &inner() noexcept { return _dev; }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
    DeviceCompose<feature::Out<T>> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::Out<int>>, AsyncDeviceCompose<feature::Out<int>>>,
      "AsyncDevice<feature::placeholder, feature::Out> is not AsyncDeviceCompose<feature::Out>");

  template <class T>
  class AsyncDevice<feature::InOut<T>> {
  public:
    template <class... Args>
    AsyncDevice(std::shared_ptr<coro::CountingSemaphore<1>> read_sem,
                std::shared_ptr<coro::CountingSemaphore<1>> write_sem, Args &&...args) noexcept
        : _read_sem(std::move(read_sem)),
          _write_sem(std::move(write_sem)),
          _dev(std::forward<Args>(args)...) {}

    AsyncDevice(AsyncDevice &&rhs) noexcept
        : _read_sem(std::move(rhs._read_sem)),
          _write_sem(std::move(rhs._write_sem)),
          _dev(std::move(rhs._dev)) {}

    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      _read_sem = std::move(rhs._read_sem);
      _write_sem = std::move(rhs._write_sem);
      _dev = std::move(rhs._dev);
      return *this;
    }

    ~AsyncDevice() noexcept {
      LOG6("AsyncDevice dtor, read use count: {}, write use count: {}", _read_sem.use_count(),
           _write_sem.use_count());
    }

    int raw() const noexcept { return _dev.raw(); }

    DeviceCompose<feature::InOut<T>> &inner() noexcept { return _dev; }

    coro::CountingSemaphore<1> &read_sem() noexcept { return *_read_sem; }

    coro::CountingSemaphore<1> &write_sem() noexcept { return *_write_sem; }

    auto split() && noexcept {
      auto [sync_r, sync_w] = std::move(this->inner()).split();
      using ReadDevice = AsyncDeviceCompose<feature::In<T>>;
      auto r = ReadDevice{std::move(_read_sem), std::move(sync_r)};
      using WriteDevice = AsyncDeviceCompose<feature::Out<T>>;
      auto w = WriteDevice{std::move(_write_sem), std::move(sync_w)};
      return std::make_tuple(std::move(r), std::move(w));
    }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _read_sem;
    std::shared_ptr<coro::CountingSemaphore<1>> _write_sem;
    DeviceCompose<feature::InOut<T>> _dev;
  };

  static_assert(
      std::is_same_v<AsyncDevice<feature::InOut<int>>, AsyncDeviceCompose<feature::InOut<int>>>,
      "AsyncDevice<feature::In, feature::Out> is not AsyncDeviceCompose<feature::In, "
      "feature::Out>");

}  // namespace impl_dev

template <class... Flags>
using Device = impl_dev::DeviceCompose<Flags...>;

template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;
XSL_SYS_IO_NE
#endif
