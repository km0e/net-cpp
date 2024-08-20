#pragma once
#ifndef XSL_SYS_RAW
#  define XSL_SYS_RAW
#  include "xsl/ai.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/io.h"
#  include "xsl/sys/sync.h"

#  include <fcntl.h>
#  include <unistd.h>

#  include <expected>
#  include <memory>
#  include <system_error>
#  include <tuple>
#  include <utility>
XSL_SYS_NB

namespace impl_dev {
  class Raii {
  public:
    Raii(int fd) noexcept : _fd(fd) {}
    Raii(Raii &&rhs) noexcept : _fd(std::exchange(rhs._fd, -1)) {}
    Raii &operator=(Raii &&rhs) noexcept {
      _fd = std::exchange(rhs._fd, -1);
      return *this;
    }
    int raw() const noexcept { return _fd; }

    ~Raii() noexcept {
      if (_fd == -1) {
        return;
      }
      LOG6("close fd: {}", _fd);
      close(_fd);
    }

  protected:
    int _fd;
  };
  class Owner {
  public:
    Owner(int fd) noexcept : _dev(std::make_shared<Raii>(fd)) {}

    template <class... Args>
    Owner(Args &&...args) noexcept : _dev(std::forward<Args>(args)...) {}

    Owner(Owner &&rhs) noexcept : _dev(std::move(rhs._dev)) {}

    Owner &operator=(Owner &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }

    ~Owner() noexcept {}

    int raw() const noexcept { return _dev->raw(); }

    std::shared_ptr<Raii> to_ownered() && noexcept { return std::move(_dev); }

  protected:
    std::shared_ptr<Raii> _dev;

    template <class Derived>
    std::tuple<std::shared_ptr<Raii>, std::shared_ptr<Raii>> split(this Derived self) noexcept {
      assert(self._dev != nullptr && "dev is nullptr");
      return {self._dev, self._dev};
    }
  };
  template <class... Flags>
  class RawDevice;

  template <class... Flags>
  using RawDeviceCompose = feature::organize_feature_flags_t<
      RawDevice<feature::Item<type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                              feature::InOut<void>>>,
      Flags...>;

  template <class... Flags>
  class AsyncRawDevice;

  template <class... Flags>
  using AsyncRawDeviceCompose = feature::organize_feature_flags_t<
      AsyncRawDevice<feature::Item<type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                                   feature::InOut<void>>,
                     feature::Dyn, feature::Own>,
      Flags...>;

  template <class Trait>
  class RawDevice<feature::In<Trait>> : public Owner {
  private:
    using Base = Owner;

  public:
    using Base::Owner;
    using device_traits_type = Trait;
    using device_features_type = feature::In<device_traits_type>;
    /**
     * @brief corresponding async device type
     * @warning derived class should implement this type
     *
     */
    using async_type = AsyncRawDeviceCompose<feature::In<device_traits_type>>;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::In<T>>, aka AsyncDevice<feature::In<T>>
     */
    inline decltype(auto) async(this auto self, Poller &poller) noexcept {
      using device_type = decltype(self)::async_type;
      auto sem = std::make_shared<CountingSemaphore<1>>();
      poller.add(self.raw(),
                 PollForCoro<typename device_traits_type::poll_traits_type, IOM_EVENTS::IN>{sem});
      return device_type{std::move(sem), std::move(self)};
    }
  };

  template <class Traits>
  class RawDevice<feature::Out<Traits>> : public Owner {
  private:
    using Base = Owner;

  public:
    using Base::Owner;
    using device_traits_type = Traits;
    using device_features_type = feature::Out<device_traits_type>;
    /**
     * @brief corresponding async device type
     * @warning derived class should implement this type
     *
     */
    using async_type = AsyncRawDeviceCompose<feature::Out<device_traits_type>>;
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::Out<T>>, aka AsyncDevice<feature::Out<T>>
     */
    inline decltype(auto) async(this auto self, Poller &poller) noexcept {
      using device_type = decltype(self)::async_type;
      auto sem = std::make_shared<CountingSemaphore<1>>();
      poller.add(self.raw(),
                 PollForCoro<typename device_traits_type::poll_traits_type, IOM_EVENTS::OUT>{sem});
      return device_type{std::move(sem), std::move(self)};
    }
  };

  template <class Traits>
  class RawDevice<feature::InOut<Traits>> : public Owner {
  private:
    using Base = Owner;

  public:
    using Base::Owner;
    using device_traits_type = Traits;
    using device_features_type = feature::InOut<device_traits_type>;
    /**
     * @brief corresponding async device type
     * @warning derived class should implement this type
     *
     */
    using async_type = AsyncRawDeviceCompose<feature::InOut<device_traits_type>>;
    /**
     * @brief rebind the device with new features
     *
     * @tparam InOut
     */
    template <template <class> class InOut>
    using rebind = RawDevice<InOut<device_traits_type>>;

    decltype(auto) split(this auto self) noexcept {
      auto [r_dev, w_dev] = std::move(self).Base::split();
      using in_type = decltype(self)::template rebind<feature::In>;
      using out_type = decltype(self)::template rebind<feature::Out>;
      auto r = in_type(std::move(r_dev));
      auto w = out_type(std::move(w_dev));
      return std::make_tuple(std::move(r), std::move(w));
    }
    /**
    @brief convert to AsyncDevice

    @param poller
    @return AsyncDeviceCompose<feature::InOut<T>>, aka AsyncDevice<feature::InOut<T>>
     */
    inline decltype(auto) async(this auto self, Poller &poller) noexcept {
      using device_type = decltype(self)::async_type;
      auto read_sem = std::make_shared<CountingSemaphore<1>>();
      auto write_sem = std::make_shared<CountingSemaphore<1>>();
      poller.add(self.raw(), PollForCoro<typename device_traits_type::poll_traits_type,
                                         IOM_EVENTS::IN, IOM_EVENTS::OUT>{read_sem, write_sem});
      return device_type{std::move(read_sem), std::move(write_sem), std::move(self).to_ownered()};
    }
  };

  template <class Traits, class T, class U>
  class AsyncRawDevice<feature::In<Traits>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::In<byte>, U>, feature::placeholder> {
    template <class...>
    friend class AsyncRawDevice;

  public:
    using device_traits_type = Traits;
    using device_features_type = feature::In<device_traits_type>;
    using dynamic_type = AsyncRawDevice<device_features_type, feature::Dyn, U>;
    using value_type = typename device_traits_type::value_type;
    using sem_type = CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<RawDevice<device_features_type>, Args...>
    AsyncRawDevice(Sem &&sem, Args &&...args) noexcept
        : _sem(std::forward<Sem>(sem)), _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncRawDevice(AsyncRawDevice<feature::In<Traits>, Flags...> &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncRawDevice(AsyncRawDevice &&rhs) noexcept = default;

    AsyncRawDevice &operator=(AsyncRawDevice &&rhs) noexcept = default;

    ~AsyncRawDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    sem_type &sem() { return *_sem; }

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> read(std::span<value_type> buf) {
      return imm_read<Executor>(*this, buf);
    }

    Task<Result> read(std::span<value_type> buf) { return this->read<>(buf); }

    template <class Dyn = dynamic_type>
    std::unique_ptr<Dyn> to_unique_dyn(this auto self) noexcept {
      using dynamic_type = decltype(self)::dynamic_type;
      return std::make_unique<dynamic_type>(std::move(self));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    RawDeviceCompose<device_features_type> _dev;
  };

  template <class Traits, class T, class U>
  class AsyncRawDevice<feature::Out<Traits>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::Out<byte>, U>, feature::placeholder> {
    template <class...>
    friend class AsyncRawDevice;

  public:
    using device_traits_type = Traits;
    using device_features_type = feature::Out<device_traits_type>;
    using dynamic_type = AsyncRawDevice<device_features_type, feature::Dyn, U>;
    using value_type = typename device_traits_type::value_type;
    using sem_type = CountingSemaphore<1>;

    template <class Sem, class... Args>
      requires std::constructible_from<std::shared_ptr<sem_type>, Sem>
                   && std::constructible_from<RawDevice<device_features_type>, Args...>
    AsyncRawDevice(Sem &&sem, Args &&...args) noexcept
        : _sem(std::forward<Sem>(sem)), _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncRawDevice(AsyncRawDevice<feature::Out<Traits>, Flags...> &&rhs) noexcept
        : _sem(std::move(rhs._sem)), _dev(std::move(rhs._dev)) {}

    AsyncRawDevice(AsyncRawDevice &&rhs) noexcept = default;

    AsyncRawDevice &operator=(AsyncRawDevice &&rhs) noexcept = default;

    ~AsyncRawDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    sem_type &sem() { return *_sem; }

    template <class Executor = coro::ExecutorBase>
    Task<Result, Executor> write(std::span<const value_type> buf) {
      return imm_write<Executor>(*this, buf);
    }

    Task<Result> write(std::span<const value_type> buf) { return this->write<>(buf); }

    template <class Dyn = dynamic_type>
    std::unique_ptr<Dyn> to_unique_dyn(this auto self) noexcept {
      using dynamic_type = decltype(self)::dynamic_type;
      return std::make_unique<dynamic_type>(std::move(self));
    }

  protected:
    std::shared_ptr<sem_type> _sem;
    RawDevice<device_features_type> _dev;
  };

  template <class Traits, class T, class U>
  class AsyncRawDevice<feature::InOut<Traits>, T, U>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>,
                                  ai::AsyncDevice<feature::InOut<byte>, U>, feature::placeholder> {
    template <class...>
    friend class AsyncRawDevice;

  public:
    using device_traits_type = Traits;
    using device_features_type = feature::InOut<device_traits_type>;
    using dynamic_type = AsyncRawDevice<device_features_type, feature::Dyn, U>;
    using value_type = typename device_traits_type::value_type;
    using sem_type = CountingSemaphore<1>;
    using inner_type = RawDeviceCompose<device_features_type>;

    template <template <class> class InOut>
    using rebind = AsyncRawDevice<InOut<device_traits_type>, T, U>;

    AsyncRawDevice() noexcept : _read_sem(nullptr), _write_sem(nullptr), _dev() {}
    template <class ReadSem, class WriteSem, class... Args>
    AsyncRawDevice(ReadSem &&read_sem, WriteSem &&write_sem, Args &&...args) noexcept
        : _read_sem(std::forward<ReadSem>(read_sem)),
          _write_sem(std::forward<WriteSem>(write_sem)),
          _dev(std::forward<Args>(args)...) {}

    template <class... Flags>
    AsyncRawDevice(AsyncRawDevice<feature::InOut<Traits>, Flags...> &&rhs) noexcept
        : _read_sem(std::move(rhs._read_sem)),
          _write_sem(std::move(rhs._write_sem)),
          _dev(std::move(rhs._dev)) {}

    AsyncRawDevice(AsyncRawDevice &&rhs) noexcept = default;

    AsyncRawDevice &operator=(AsyncRawDevice &&rhs) noexcept = default;

    ~AsyncRawDevice() noexcept {}

    decltype(auto) raw() { return _dev.raw(); }

    inner_type &inner() { return _dev; }

    sem_type &read_sem() { return *_read_sem; }

    sem_type &write_sem() { return *_write_sem; }

    Task<Result> read(std::span<byte> buf) { return imm_read<coro::ExecutorBase>(*this, buf); }

    Task<Result> write(std::span<const byte> buf) {
      return imm_write<coro::ExecutorBase>(*this, buf);
    }

    template <class Dyn = dynamic_type>
    std::unique_ptr<Dyn> to_unique_dyn(this auto self) noexcept {
      using dynamic_type = decltype(self)::dynamic_type;
      return std::make_unique<dynamic_type>(std::move(self));
    }

    /**
     * @brief split the device into two devices
     *
     * @return std::tuple<AsyncDevice<feature::In<socket_traits_type>, T, U>,
     * AsyncDevice<feature::Out<socket_traits_type>, T, U>>
     */
    decltype(auto) split(this auto self) noexcept {
      auto [r, w] = std::move(self._dev).split();
      using in_type = decltype(self)::template rebind<feature::In>;
      using out_type = decltype(self)::template rebind<feature::Out>;
      auto _in = in_type{std::move(self._read_sem), std::move(r)};
      auto _out = out_type{std::move(self._write_sem), std::move(w)};
      return std::make_tuple(std::move(_in), std::move(_out));
    }

  protected:
    std::shared_ptr<sem_type> _read_sem, _write_sem;
    RawDeviceCompose<device_features_type> _dev;
  };
}  // namespace impl_dev

struct DefaultDeviceTraits {
  using value_type = byte;
  using poll_traits_type = DefaultPollTraits;
};

/**
 * @brief the raw device
 *
 * @tparam Flags the flags of the device, currently only support DeviceTraits
 */
template <class... Flags>
using RawDevice = impl_dev::RawDeviceCompose<Flags...>;

template <class... Flags>
using AsyncRawDevice = impl_dev::AsyncRawDeviceCompose<Flags...>;

template <bool is_blocking>
std::expected<void, std::errc> set_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return std::unexpected{std::errc(errno)};
  }
  if constexpr (!is_blocking) {
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      return std::unexpected{std::errc(errno)};
    }
  } else {
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
      return std::unexpected{std::errc(errno)};
    }
  }
  return {};
}
std::expected<void, std::errc> set_blocking(int fd, bool blocking);

template <class F, class... Args>
int filter_interrupt(F &&f, Args &&...args) {
  int ret;
  do {
    ret = f(std::forward<Args>(args)...);
  } while (ret == -1 && errno == EINTR);
  return ret;
}

XSL_SYS_NE
#endif
