#pragma once
#ifndef XSL_AI_DEV
#  define XSL_AI_DEV
#  include "xsl/ai/def.h"
#  include "xsl/coro.h"
#  include "xsl/feature.h"

#  include <cstddef>
#  include <optional>
#  include <span>
#  include <tuple>
#  include <utility>

XSL_AI_NB

using Result = std::tuple<std::size_t, std::optional<std::errc>>;

template <class Device, class T>
concept ReadDeviceLike = requires(Device t, std::span<T> buf) {
  { t.read(buf) } -> std::same_as<Result>;
};

template <class Executor, class Device, class... Args>
  requires(!requires(Device t, Args... args) {
    { t.template read<Executor>(args...) } -> std::same_as<Task<Result>>;
  })
constexpr decltype(auto) read_poly_resolve(Device&& dev, Args&&... args) {
  return dev.read(std::forward<Args>(args)...);
}

template <class Executor, class Device, class... Args>
  requires requires(Device t, Args... args) {
    { t.template read<Executor>(args...) } -> std::same_as<Task<Result>>;
  }
constexpr decltype(auto) read_poly_resolve(Device&& dev, Args&&... args) {
  return dev.template read<Executor>(std::forward<Args>(args)...);
}

template <class Device, class T>
concept WriteDeviceLike = requires(Device t, std::span<const T> buf) {
  { t.write(buf) } -> std::same_as<Result>;
};

template <class Executor, class Device, class... Args>
constexpr decltype(auto) write_poly_resolve(Device&& dev, Args&&... args) {
  if constexpr (std::is_same_v<Executor, coro::ExecutorBase>) {
    return dev.write(std::forward<Args>(args)...);
  } else {
    return dev.template write<Executor>(std::forward<Args>(args)...);
  }
}

template <class Device, class T>
concept AsyncReadDeviceLike = requires(Device t, std::span<T> buf) {
  { t.read(buf) } -> std::same_as<Task<Result>>;
};

template <class Device>
concept ABRL = AsyncReadDeviceLike<Device, byte>;

template <class Device, class T>
concept AsyncWriteDeviceLike = requires(Device t, std::span<const T> buf) {
  { t.write(buf) } -> std::same_as<Task<Result>>;
};

template <class Device>
concept ABWL = AsyncWriteDeviceLike<Device, byte>;

template <class Device, class T>
concept AsyncReadWriteDeviceLike
    = AsyncReadDeviceLike<Device, T> && AsyncWriteDeviceLike<Device, T>;

template <class Device>
concept ABRWL = AsyncReadWriteDeviceLike<Device, byte>;

namespace impl_dev {
  template <class... Flags>
  class AsyncDevice;

  template <class... Flags>
  using AsyncDeviceCompose = feature::organize_feature_flags_t<
      AsyncDevice<feature::Item<type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                                feature::InOut<void>>,
                  feature::Own>,
      Flags...>;

  template <class T>
  class AsyncDevice<feature::In<T>, feature::Own>;

  template <class T>
  class AsyncDevice<feature::In<T>, feature::placeholder> {
  public:
    using value_type = T;
    using dynamic_type = AsyncDevice;
    virtual ~AsyncDevice() = default;
    /**
    @brief read data from the device

    @param buf the buffer to indicate the data
    @return Task<Result> the result of the read, the
    first element is the size of the data read, the second element is the error code, if there is
    error,
     */
    virtual Task<Result> read(std::span<value_type> buf [[maybe_unused]]) { std::unreachable(); }
  };

  template <class T>
  class AsyncDevice<feature::Out<T>, feature::placeholder> {
  public:
    using value_type = T;
    using dynamic_type = AsyncDevice;
    virtual ~AsyncDevice() = default;
    virtual Task<Result> write(std::span<const value_type> buf [[maybe_unused]]) {
      std::unreachable();
    }
  };
  template <class Traits>
  class AsyncDevice<feature::InOut<Traits>, feature::placeholder>
      : public AsyncDevice<feature::In<Traits>, feature::placeholder>,
        public AsyncDevice<feature::Out<Traits>, feature::placeholder> {};
}  // namespace impl_dev
/**
 * @brief AsyncDevice
 *
 * @tparam Flags the flags of the device, currently only support DeviceTraits
 */
template <class... Flags>
using AsyncDevice = impl_dev::AsyncDeviceCompose<Flags...>;

using ABR = AsyncDevice<feature::In<byte>>;

using ABW = AsyncDevice<feature::Out<byte>>;

template <class T>
class AsyncWritable {
public:
  using value_type = T;
  virtual ~AsyncWritable() {}
  virtual Task<Result> write(AsyncDevice<feature::Out<value_type>>& awd) = 0;
};
template <class T>
class AsyncReadable {
public:
  using value_type = T;
  virtual ~AsyncReadable() {}
  virtual Task<Result> read(AsyncDevice<feature::In<value_type>>& ard) = 0;
};
XSL_AI_NE
#endif
