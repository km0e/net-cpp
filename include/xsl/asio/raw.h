/**
 * @file raw.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Asynchronous raw IO operations
 * @version 0.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_ASIO_RAW
#  define XSL_ASIO_RAW
#  include "xsl/asio/def.h"
#  include "xsl/coro.h"
#  include "xsl/io.h"
#  include "xsl/sys.h"

#  include <sys/epoll.h>

#  include <tuple>
#  include <utility>

XSL_ASIO_NB

using io::IOM_EVENTS;

template <IOM_EVENTS... Events>
using AsyncOwnerStorage
    = std::tuple<RawOwner, PubSub<StaticExactPubSubStorage<IOM_EVENTS, SPSCSignal2<1>, Events...>>>;

template <IOM_EVENTS... Events>
struct AsyncOwner {
  using storage_type = AsyncOwnerStorage<Events...>;
  std::shared_ptr<storage_type> storage;

  template <typename Traits>
  AsyncOwner(RawOwner &&raw, Poller &poller, Traits tag)
      : storage(std::make_shared<storage_type>()) {
    std::get<0>(*storage) = std::move(raw);
    poller.add(this->raw(), (IOM_EVENTS::ET | ... | Events),
               io::PollForCoro<Traits, std::weak_ptr<storage_type>>{tag, storage});
  }

  constexpr auto &&raw(this auto &&self) noexcept {
    return std::get<0>(*std::forward<decltype(self)>(self).storage).raw();
  }

  constexpr auto &&read_signal(this auto &&self) noexcept {
    return *std::get<1>(*std::forward<decltype(self)>(self).storage)
                .template signal<IOM_EVENTS::IN>();
  }

  constexpr auto &&write_signal(this auto &&self) noexcept {
    return *std::get<1>(*std::forward<decltype(self)>(self).storage)
                .template signal<IOM_EVENTS::OUT>();
  }

  constexpr bool is_valid() const noexcept { return std::get<0>(*storage).is_valid(); }
};

template <typename Traits, IOM_EVENTS... Events>
  requires(sizeof...(Events) > 0)
AsyncOwner(RawOwner &&raw, Poller &poller, Traits tag) -> AsyncOwner<Events...>;

struct AsyncUtil {
  template <class _Self>
    requires(!std::is_reference_v<_Self>)
  constexpr decltype(auto) async(this _Self &&self, Poller &poller) noexcept {
    using poll_traits_type = typename _Self::poll_traits_type;
    return typename _Self::async_type{AsyncOwner<IOM_EVENTS::IN, IOM_EVENTS::OUT>(
        std::move(self).into_owner(), poller, poll_traits_type{})};
  }
};

XSL_ASIO_NE

XSL_IO_NB
template <IOM_EVENTS... Events>
struct PollHandlerTraits<std::weak_ptr<_asio::AsyncOwnerStorage<Events...>>> {
  static constexpr _coro::PubSub<StaticExactPubSubStorage<IOM_EVENTS, SPSCSignal2<1>, Events...>> *
  pubsub(const std::weak_ptr<_asio::AsyncOwnerStorage<Events...>> &storage) {
    if (auto ptr = storage.lock()) {
      return &std::get<1>(*ptr);  // Assuming the second element is the PubSub2 instance
    } else {
      return nullptr;  // Handle the case where the weak_ptr is expired
    }
  }
};
XSL_IO_NE
#endif
