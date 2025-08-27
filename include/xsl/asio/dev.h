/**
 * @file dev.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Async device
 * @version 0.3.0
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_DEV
#  define XSL_ASIO_DEV
#  include <xsl/asio/def.h>
#  include <xsl/asio/io.h>
#  include <xsl/io/context.h>
#  include <xsl/type_traits.h>

XSL_ASIO_NB

using io::IOM_EVENTS;

template <IOM_EVENTS... Events>
using AsyncDeviceStorage
    = std::tuple<RawOwner, PubSub<StaticExactPubSubStorage<IOM_EVENTS, SPSCSignal2<1>, Events...>>>;

template <IOM_EVENTS... Events, class Traits>
constexpr Expected<void, errc> init_async_device(Context &ctx,
                                                 std::shared_ptr<AsyncDeviceStorage<Events...>> &s,
                                                 Traits tag) {
  ENSEC(ctx.add(std::get<0>(*s).raw(), (IOM_EVENTS::ET | ... | Events),
                PollForCoro<Traits, std::weak_ptr<AsyncDeviceStorage<Events...>>>{tag, s}));
  return {};
}

struct AsyncUtil {
  template <class _Self>
    requires(!std::is_reference_v<_Self>)
  constexpr decltype(auto) async(this _Self &&self, Context &ctx) noexcept {
    using poll_traits_type = typename _Self::poll_traits_type;
    return typename _Self::async_type{AsyncDevice<IOM_EVENTS::IN, IOM_EVENTS::OUT>(
        std::move(self).into_owner(), ctx, poll_traits_type{})};
  }
};

namespace detail {

  template <class T>
  concept IsAD = requires(T t) {
    { t.async_dev_storage() };
  };
  /**
   * @brief Check if T has async_dev_storage method which returns a type that has event e
   *
   * @tparam T
   */
  template <class T, IOM_EVENTS e>
  concept IsADW = IsAD<T> && requires(T t) {
    { find_value_v<e, std::remove_cvref_t<decltype(t.async_dev_storage())>> };
  };
}  // namespace detail

struct AsyncDeviceUtil {
  /**
   * @brief Get the raw fd
   *
   * @return RawOwner&
   */
  constexpr auto &&raw(this detail::IsAD auto &&self) noexcept {
    return std::get<0>(std::forward<decltype(self)>(self).async_dev_storage()).raw();
  }
  /**
   * @brief Get the read signal
   *
   * @return SPSCSignal2<1>&
   */
  constexpr auto &&read_signal(this detail::IsADW<IOM_EVENTS::IN> auto &&self) noexcept {
    return *std::get<1>(std::forward<decltype(self)>(self).async_dev_storage())
                .template signal<IOM_EVENTS::IN>();
  }

  /**
   * @brief Get the write signal
   *
   * @return SPSCSignal2<1>&
   */
  constexpr auto &&write_signal(this detail::IsADW<IOM_EVENTS::OUT> auto &&self) noexcept {
    return *std::get<1>(std::forward<decltype(self)>(self).async_dev_storage())
                .template signal<IOM_EVENTS::OUT>();
  }
};
template <class Events, class Util>
class AsyncDevice;

/**
 * @brief Async device
 */
template <IOM_EVENTS... Events, class Util>
class AsyncDevice<_value_pack<Events...>, Util> : public AsyncDeviceUtil, public Util {
public:
  using storage_type = AsyncDeviceStorage<Events...>;

  using value_type = byte;
  AsyncDevice(std::shared_ptr<storage_type> &&storage) : storage(std::move(storage)) {}
  AsyncDevice(AsyncDevice &&) noexcept = default;
  AsyncDevice &operator=(AsyncDevice &&) noexcept = default;
  AsyncDevice(const AsyncDevice &) = default;
  AsyncDevice &operator=(const AsyncDevice &) = default;

protected:
  std::shared_ptr<storage_type> storage;

  auto &&async_dev_storage(this auto &&self) {
    return std::forward<like_t<decltype(self), storage_type>>(*self.storage);
  }

  friend struct AsyncDeviceUtil;
};

/**
 * @brief Make an async device
 *
 * @tparam Util Some utility class, such as AsyncSocketUtil
 * @param ctx the context
 * @param r the raw owner
 * @return Expected<AsyncDevice<_value_pack<Events...>, Util>, errc>
 */
template <IOM_EVENTS... Events, class Util>
constexpr Expected<AsyncDevice<_value_pack<Events...>, Util>, errc> make_async_device(Context &ctx,
                                                                                      RawOwner &&r,
                                                                                      Util) {
  auto ptr = std::make_shared<AsyncDeviceStorage<Events...>>();
  std::get<0>(*ptr) = std::move(r);
  ENSEC(init_async_device(ctx, ptr, typename Util::poll_traits_type{}));
  return {{std::move(ptr)}};
}

XSL_ASIO_NE

XSL_IO_NB
template <IOM_EVENTS... Events>
struct PollHandlerTraits<std::weak_ptr<_asio::AsyncDeviceStorage<Events...>>> {
  static constexpr coro::PubSub<StaticExactPubSubStorage<IOM_EVENTS, SPSCSignal2<1>, Events...>> *
  pubsub(const std::weak_ptr<_asio::AsyncDeviceStorage<Events...>> &storage) {
    if (auto ptr = storage.lock()) {
      return &std::get<1>(*ptr);  // Assuming the second element is the PubSub2 instance
    } else {
      return nullptr;  // Handle the case where the weak_ptr is expired
    }
  }
};
XSL_IO_NE
#endif
