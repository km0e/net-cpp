#pragma once
#ifndef XSL_SYS_IO_DEV
#  define XSL_SYS_IO_DEV
#  include "xsl/coro/semaphore.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/sync.h"
#  include "xsl/sys/io/def.h"

#  include <unistd.h>

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

template <class... Flags>
class AsyncDevice;
using AsyncReadDevice = AsyncDevice<feature::In, feature::placeholder>;
using AsyncWriteDevice = AsyncDevice<feature::placeholder, feature::Out>;
using AsyncReadWriteDevice = AsyncDevice<feature::In, feature::Out>;

template <class... Flags>
class Device;
using ReadDevice = Device<feature::In, feature::placeholder>;
using WriteDevice = Device<feature::placeholder, feature::Out>;
using ReadWriteDevice = Device<feature::In, feature::Out>;

template <>
class Device<feature::placeholder, feature::placeholder> {
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

template <>
class Device<feature::In, feature::placeholder>
    : public Device<feature::placeholder, feature::placeholder> {
private:
  using Base = Device<feature::placeholder, feature::placeholder>;

public:
  using Base::Base;
  AsyncReadDevice async(std::shared_ptr<sync::Poller> &poller) && noexcept;
};

template <>
class Device<feature::placeholder, feature::Out>
    : public Device<feature::placeholder, feature::placeholder> {
private:
  using Base = Device<feature::placeholder, feature::placeholder>;

public:
  using Base::Base;
  AsyncWriteDevice async(std::shared_ptr<sync::Poller> &poller) && noexcept;
};

template <>
class Device<feature::In, feature::Out>
    : public Device<feature::placeholder, feature::placeholder> {
private:
  using Base = Device<feature::placeholder, feature::placeholder>;

public:
  using Base::Base;
  std::tuple<ReadDevice, WriteDevice> split() && noexcept {
    return {ReadDevice{std::move(_dev)}, WriteDevice{std::move(_dev)}};
  }

  AsyncReadWriteDevice async(std::shared_ptr<sync::Poller> &poller) && noexcept;
};

template <>
class AsyncDevice<feature::In, feature::placeholder>
    : public Device<feature::In, feature::placeholder> {
public:
  AsyncDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
      : Device(std::move(dev)), _sem(std::move(sem)) {}
  AsyncDevice(std::shared_ptr<NativeDevice> dev,
              std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
      : Device(std::move(dev)), _sem(std::move(sem)) {}
  AsyncDevice(AsyncDevice &&rhs) noexcept : Device(std::move(rhs)), _sem(std::move(rhs._sem)) {}
  AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
    Device::operator=(std::move(rhs));
    _sem = std::move(rhs._sem);
    return *this;
  }

  coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

protected:
  std::shared_ptr<coro::CountingSemaphore<1>> _sem;
};

inline AsyncDevice<feature::In, feature::placeholder> Device<
    feature::In, feature::placeholder>::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
  auto sem = std::make_shared<coro::CountingSemaphore<1>>();
  poller->add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::IN>{sem});
  return {std::move(_dev), std::move(sem)};
}

template <>
class AsyncDevice<feature::placeholder, feature::Out>
    : public Device<feature::placeholder, feature::Out> {
public:
  AsyncDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
      : Device(std::move(dev)), _sem(std::move(sem)) {}
  AsyncDevice(std::shared_ptr<NativeDevice> dev,
              std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
      : Device(std::move(dev)), _sem(std::move(sem)) {}
  AsyncDevice(AsyncDevice &&rhs) noexcept : Device(std::move(rhs)), _sem(std::move(rhs._sem)) {}
  AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
    Device::operator=(std::move(rhs));
    _sem = std::move(rhs._sem);
    return *this;
  }

  coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

protected:
  std::shared_ptr<coro::CountingSemaphore<1>> _sem;
};

inline AsyncDevice<feature::placeholder, feature::Out> Device<
    feature::placeholder, feature::Out>::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
  auto sem = std::make_shared<coro::CountingSemaphore<1>>();
  poller->add(_dev->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
  return {std::move(_dev), std::move(sem)};
}

template <>
class AsyncDevice<feature::In, feature::Out> : public Device<feature::In, feature::Out> {
public:
  AsyncDevice(std::shared_ptr<NativeDevice> dev,
              std::shared_ptr<coro::CountingSemaphore<1>> read_sem,
              std::shared_ptr<coro::CountingSemaphore<1>> write_sem) noexcept
      : Device(std::move(dev)), _read_sem(std::move(read_sem)), _write_sem(std::move(write_sem)) {}
  AsyncDevice(AsyncDevice &&rhs) noexcept
      : Device(std::move(rhs)),
        _read_sem(std::move(rhs._read_sem)),
        _write_sem(std::move(rhs._write_sem)) {}
  AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
    Device::operator=(std::move(rhs));
    _read_sem = std::move(rhs._read_sem);
    _write_sem = std::move(rhs._write_sem);
    return *this;
  }

  coro::CountingSemaphore<1> &read_sem() noexcept { return *_read_sem; }
  coro::CountingSemaphore<1> &write_sem() noexcept { return *_write_sem; }

  std::tuple<AsyncReadDevice, AsyncWriteDevice> split() && noexcept {
    return {AsyncReadDevice{_dev, std::move(_read_sem)},
            AsyncWriteDevice{_dev, std::move(_write_sem)}};
  }

protected:
  std::shared_ptr<coro::CountingSemaphore<1>> _read_sem;
  std::shared_ptr<coro::CountingSemaphore<1>> _write_sem;
};

inline AsyncDevice<feature::In, feature::Out> Device<feature::In, feature::Out>::async(
    std::shared_ptr<sync::Poller> &poller) && noexcept {
  auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
  auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
  poller->add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{read_sem, write_sem});
  return {std::move(_dev), std::move(read_sem), std::move(write_sem)};
}
XSL_SYS_IO_NE
#endif
