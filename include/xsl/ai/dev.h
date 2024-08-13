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

XSL_AI_NB
using Result = std::tuple<std::size_t, std::optional<std::errc>>;
namespace impl_dev {
  template <class... Flags>
  class AsyncDevice;

  template <class T>
  class AsyncDevice<feature::In<T>, feature::Own>;

  template <class T>
  class AsyncDevice<feature::In<T>, feature::placeholder> {
  public:
    virtual ~AsyncDevice() = default;
    /**
    @brief read data from the device

    @param buf the buffer to indicate the data
    @return coro::Task<Result> the result of the read, the
    first element is the size of the data read, the second element is the error code, if there is
    error,
     */
    virtual coro::Task<Result> read(std::span<T> buf) = 0;
  };

  template <class T>
  class AsyncDevice<feature::Out<T>, feature::placeholder> {
  public:
    virtual ~AsyncDevice() = default;
    virtual coro::Task<Result> write(std::span<const T> buf) = 0;
  };

  template <class T>
  class AsyncDevice<feature::InOut<T>, feature::placeholder>
      : public AsyncDevice<feature::In<T>, feature::placeholder>,
        public AsyncDevice<feature::Out<T>, feature::placeholder> {};
}  // namespace impl_dev

template <class Device, class T>
concept ReadDeviceLike = requires(Device t, std::span<T> buf) {
  { t.read(buf) } -> std::same_as<Result>;
};

template <class Device, class T>
concept WriteDeviceLike = requires(Device t, std::span<const T> buf) {
  { t.write(buf) } -> std::same_as<Result>;
};

template <class Device, class T>
concept AsyncReadDeviceLike = requires(Device t, std::span<T> buf) {
  { t.read(buf) } -> std::same_as<coro::Task<Result>>;
};

template <class Device, class T>
concept AsyncWriteDeviceLike = requires(Device t, std::span<const T> buf) {
  { t.write(buf) } -> std::same_as<coro::Task<Result>>;
};

template <class Device, class T>
concept AsyncReadWriteDeviceLike
    = AsyncReadDeviceLike<Device, T> && AsyncWriteDeviceLike<Device, T>;

template <class... Flags>
using AsyncDevice = feature::organize_feature_flags_t<
    impl_dev::AsyncDevice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>,
                                        feature::Out<void>, feature::InOut<void>>,
                          feature::Own>,
    Flags...>;

template <class T>
class AsyncWritable {
public:
  virtual ~AsyncWritable() {}
  virtual coro::Task<Result> write(ai::AsyncDevice<feature::Out<T>>& awd) = 0;
};
template <class T>
class AsyncReadable {
public:
  virtual ~AsyncReadable() {}
  virtual coro::Task<Result> read(ai::AsyncDevice<feature::In<T>>& ard) = 0;
};
XSL_AI_NE
#endif
