#pragma once
#ifndef XSL_AI_DEV
#  define XSL_AI_DEV
#  include "xsl/ai/def.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"

#  include <cstddef>
#  include <optional>
#  include <span>
#  include <tuple>

XSL_AI_NB
namespace impl_dev {
  template <class... Flags>
  class AsyncDevice;

  template <>
  class AsyncDevice<feature::In, feature::Own>;

  template <>
  class AsyncDevice<feature::In, feature::placeholder> {
  public:
    virtual ~AsyncDevice() = default;
    virtual coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> read(
        std::span<std::byte> buf)
        = 0;
  };

  template <>
  class AsyncDevice<feature::Out, feature::placeholder> {
  public:
    virtual ~AsyncDevice() = default;
    virtual coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> write(
        std::span<const std::byte> buf)
        = 0;
  };

  template <>
  class AsyncDevice<feature::placeholder, feature::placeholder>
      : public AsyncDevice<feature::In, feature::placeholder>,
        public AsyncDevice<feature::Out, feature::placeholder> {};
}  // namespace impl_dev

template <class... Flags>
using AsyncDevice = feature::origanize_feature_flags_t<
    impl_dev::AsyncDevice<feature::set<feature::In, feature::Out>, feature::Own>, Flags...>;

XSL_AI_NE
#endif
