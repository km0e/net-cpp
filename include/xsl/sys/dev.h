/**
 * @file dev.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Raw device
 * @version 0.1
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_RAW_DEV
#  define XSL_SYS_RAW_DEV
#  include "xsl/coro.h"
#  include "xsl/def.h"
#  include "xsl/feature.h"
#  include "xsl/io.h"
#  include "xsl/io/def.h"
#  include "xsl/sys/io.h"
#  include "xsl/sys/sync.h"

#  include <utility>
XSL_SYS_NB
struct RawAsyncReadDevice;
struct RawAsyncWriteDevice;
struct RawAsyncReadWriteDevice;

/// @brief RawOwner is a wrapper for file descriptor
struct RawOwner {
  int fd;

  constexpr RawOwner(int fd) noexcept : fd(fd) {}
  constexpr RawOwner(RawOwner &&rhs) noexcept : fd(std::exchange(rhs.fd, -1)) {}
  constexpr RawOwner &operator=(RawOwner &&rhs) noexcept {
    fd = std::exchange(rhs.fd, -1);
    return *this;
  }

  constexpr int raw() const noexcept { return fd; }
  constexpr int into_raw() && noexcept { return std::exchange(fd, -1); }
};
/// @brief RawReadDevice is a wrapper for read-only file descriptor
struct RawReadDevice : public RawOwner {
  using RawOwner::RawOwner;
  constexpr decltype(auto) async(this auto self, Poller &poller) noexcept {
    using io_traits_type = IOTraits<decltype(self)>;
    using poll_traits_type = typename decltype(self)::poll_traits_type;
    auto signal = poll_by_signal<poll_traits_type>(poller, self.raw(), IOM_EVENTS::IN);
    return typename io_traits_type::async_type{std::move(self).into_raw(), std::move(signal)};
  }
};
/// @brief RawWriteDevice is a wrapper for write-only file descriptor
struct RawWriteDevice : public RawOwner {
  using RawOwner::RawOwner;

  constexpr decltype(auto) async(this auto self, Poller &poller) noexcept {
    using io_traits_type = IOTraits<decltype(self)>;
    using poll_traits_type = typename decltype(self)::poll_traits_type;
    auto signal = poll_by_signal<poll_traits_type>(poller, self.raw(), IOM_EVENTS::OUT);
    return typename io_traits_type::async_type{std::move(self).into_raw(), std::move(signal)};
  }
};
/// @brief RawReadWriteDevice is a wrapper for read-write file descriptor
struct RawReadWriteDevice : public RawOwner {
  using RawOwner::RawOwner;

  template <class _Self>
  constexpr decltype(auto) async(this _Self self, Poller &poller) noexcept {
    using poll_traits_type = typename _Self::poll_traits_type;
    using async_type = typename _Self::async_type;
    auto [r_sig, w_sig]
        = poll_by_signal<poll_traits_type>(poller, self.raw(), IOM_EVENTS::IN, IOM_EVENTS::OUT);
    return async_type(std::move(self).into_raw(), std::move(r_sig), std::move(w_sig));
  }
};
/// @brief RawAsyncReadDevice is a wrapper for read-only file descriptor with async support
struct RawAsyncReadDevice : public RawReadDevice {
  using Base = RawReadDevice;

  SignalReceiver<> read_signal;

  constexpr RawAsyncReadDevice(int fd, auto &&signal) noexcept
      : Base(fd), read_signal(std::forward<decltype(signal)>(signal)) {}
  constexpr RawAsyncReadDevice(RawAsyncReadDevice &&rhs) noexcept
      : Base(std::move(rhs)), read_signal(std::move(rhs.read_signal)) {}
  constexpr RawAsyncReadDevice &operator=(RawAsyncReadDevice &&rhs) noexcept {
    read_signal = std::move(rhs.read_signal);
    return *this;
  }

  constexpr Task<io::Result> read(std::span<byte> buf) { return _sys::read(fd, buf, read_signal); }
};
/// @brief RawAsyncWriteDevice is a wrapper for write-only file descriptor with async support
struct RawAsyncWriteDevice : public RawWriteDevice {
  using Base = RawWriteDevice;

  SignalReceiver<> write_signal;

  constexpr RawAsyncWriteDevice(int fd, auto &&signal) noexcept
      : Base(fd), write_signal(std::forward<decltype(signal)>(signal)) {}
  constexpr RawAsyncWriteDevice(RawAsyncWriteDevice &&rhs) noexcept
      : Base(std::move(rhs)), write_signal(std::move(rhs.write_signal)) {}
  constexpr RawAsyncWriteDevice &operator=(RawAsyncWriteDevice &&rhs) noexcept {
    write_signal = std::move(rhs.write_signal);
    return *this;
  }

  constexpr Task<io::Result> write(std::span<const byte> buf) {
    return _sys::write(fd, buf, write_signal);
  }
};
/// @brief RawAsyncReadWriteDevice is a wrapper for read-write file descriptor with async support
struct RawAsyncReadWriteDevice : public RawReadWriteDevice {
  using Base = RawReadWriteDevice;

  SignalReceiver<> read_signal;
  SignalReceiver<> write_signal;

  constexpr RawAsyncReadWriteDevice(int fd, SignalReceiver<> &&read_signal,
                                    SignalReceiver<> &&write_signal)
      : Base(fd), read_signal(std::move(read_signal)), write_signal(std::move(write_signal)) {}

  constexpr RawAsyncReadWriteDevice(RawAsyncReadWriteDevice &&rhs) noexcept
      : Base(std::move(rhs)),
        read_signal(std::move(rhs.read_signal)),
        write_signal(std::move(rhs.write_signal)) {}
  constexpr RawAsyncReadWriteDevice &operator=(RawAsyncReadWriteDevice &&rhs) noexcept {
    read_signal = std::move(rhs.read_signal);
    write_signal = std::move(rhs.write_signal);
    return *this;
  }

  template <class _Self>
  constexpr decltype(auto) split(this _Self self) noexcept {
    using in_type = typename _Self::template rebind<In>;
    using out_type = typename _Self::template rebind<Out>;

    return std::make_pair<in_type, out_type>({self.fd, std::move(self.read_signal)},
                                             {self.fd, std::move(self.write_signal)});
  }

  constexpr Task<io::Result> read(std::span<byte> buf) { return _sys::read(fd, buf, read_signal); }

  constexpr Task<io::Result> write(std::span<const byte> buf) {
    return _sys::write(fd, buf, write_signal);
  }
};

template <class... Flags>
struct RawDeviceSelector;

template <class... Flags>
struct RawDeviceSelector<In<byte>, Flags...> {
  using type = RawReadDevice;
};

template <class... Flags>
struct RawDeviceSelector<Out<byte>, Flags...> {
  using type = RawWriteDevice;
};

template <class... Flags>
struct RawDeviceSelector<InOut<byte>, Flags...> {
  using type = RawReadWriteDevice;
};

template <class... Flags>
using RawDeviceCompose = organize_feature_flags_t<
    RawDeviceSelector<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>, Flags...>;

template <class... Flags>
struct RawAsyncDeviceSelector;

template <class... Flags>
struct RawAsyncDeviceSelector<In<byte>, Flags...> {
  using type = RawAsyncReadDevice;
};

template <class... Flags>
struct RawAsyncDeviceSelector<Out<byte>, Flags...> {
  using type = RawAsyncWriteDevice;
};

template <class... Flags>
struct RawAsyncDeviceSelector<InOut<byte>, Flags...> {
  using type = RawAsyncReadWriteDevice;
};

template <class... Flags>
using RawAsyncDeviceCompose = organize_feature_flags_t<
    RawAsyncDeviceSelector<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>, Flags...>;

/// @brief RawDevice is a wrapper for file descriptor
template <class... Flags>
using RawDevice = RawDeviceCompose<Flags...>::type;
/// @brief RawReadDevice is a wrapper for read-only file descriptor
template <class... Flags>
using RawAsyncDevice = RawAsyncDeviceCompose<Flags...>::type;

XSL_SYS_NE
XSL_IO_NB

template <>
struct IOTraits<_sys::RawReadDevice> {
  using value_type = byte;
  using device_type = _sys::RawReadDevice;
  using async_type = _sys::RawAsyncDeviceCompose<In<byte>>::type;

  static io::Result read(device_type &, std::span<byte>) { std::unreachable(); }
};

template <>
struct IOTraits<_sys::RawWriteDevice> {
  using value_type = byte;
  using device_type = _sys::RawWriteDevice;
  using async_type = _sys::RawAsyncDeviceCompose<Out<byte>>::type;

  static io::Result write(device_type &, std::span<const byte>) { std::unreachable(); }
};

template <>
struct IOTraits<_sys::RawReadWriteDevice> {
  using value_type = byte;
  using device_type = _sys::RawReadWriteDevice;
  using async_type = _sys::RawAsyncDeviceCompose<InOut<byte>>::type;

  static io::Result read(device_type &, std::span<byte>) { std::unreachable(); }

  static io::Result write(device_type &, std::span<const byte>) { std::unreachable(); }
};

template <>
struct AIOTraits<_sys::RawAsyncReadDevice> {
  using value_type = byte;
  using device_type = _sys::RawAsyncReadDevice;

  static constexpr Task<Result> read(device_type &dev, std::span<byte> buf) {
    return dev.read(buf);
  }
};

template <>
struct AIOTraits<_sys::RawAsyncWriteDevice> {
  using value_type = byte;
  using device_type = _sys::RawAsyncWriteDevice;

  static constexpr Task<Result> write(device_type &dev, std::span<const byte> buf) {
    return dev.write(buf);
  }

  static constexpr Task<Result> write_file(device_type &dev, _sys::WriteFileHint &&hint) {
    return _sys::write_file(dev, std::move(hint));
  }
};

template <>
struct AIOTraits<_sys::RawAsyncReadWriteDevice> {
  using value_type = byte;
  using device_type = _sys::RawAsyncReadWriteDevice;

  static constexpr Task<Result> read(device_type &dev, std::span<byte> buf) {
    return dev.read(buf);
  }

  static constexpr Task<Result> write(device_type &dev, std::span<const byte> buf) {
    return dev.write(buf);
  }
};
XSL_IO_NE
#endif
