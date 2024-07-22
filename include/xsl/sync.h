#pragma once
#ifndef XSL_SYNC
#  define XSL_SYNC
#  include "xsl/coro/semaphore.h"
#  include "xsl/def.h"
#  include "xsl/logctl.h"
#  include "xsl/sync/mutex.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sync/spsc.h"

#  include <array>
#  include <cstdint>
XSL_NB
using sync::IOM_EVENTS;
using sync::LockGuard;
using sync::poll_add_shared;
using sync::poll_add_unique;
using sync::Poller;
using sync::PollHandleHint;
using sync::PollHandleHintTag;
using sync::PollHandler;
using sync::ShrdGuard;
using sync::ShrdRes;
using sync::SPSC;

namespace sync {
  template <IOM_EVENTS... Events>
  class PollCallback {
  public:
    PollCallback(std::array<std::shared_ptr<coro::CountingSemaphore<1>>, sizeof...(Events)> sems)
        : sems(std::move(sems)) {}

    template <class... Sems>
    PollCallback(Sems &&...sems) : sems{std::forward<Sems>(sems)...} {}
    sync::PollHandleHint operator()(int, sync::IOM_EVENTS events) {
      LOG5("Poll Event: {}", static_cast<uint32_t>(events));
      if (!events || !!(events & sync::IOM_EVENTS::HUP)) {
        for (auto &sem : sems) {
          auto raw = sem.get();
          sem.reset();
          raw->release();
        }
        return sync::PollHandleHintTag::DELETE;
      } else {
        return handle_event(std::make_index_sequence<sizeof...(Events)>{}, events)
                   ? sync::PollHandleHintTag::DELETE
                   : sync::PollHandleHintTag::NONE;
      }
    }

  private:
    std::array<std::shared_ptr<coro::CountingSemaphore<1>>, sizeof...(Events)> sems;

    template <std::size_t... I>
    bool handle_event(std::index_sequence<I...>, sync::IOM_EVENTS events) {
      return (handle_event<Events, I>(events) && ...);
    }

    template <IOM_EVENTS E, std::size_t I>
    bool handle_event(sync::IOM_EVENTS events) {
      if (!sems[I].unique()) {
        if (!!(events & E)) {
          sems[I]->release();
        }
        return false;
      }
      return true;
    }
  };
}  // namespace sync
XSL_NE
#endif
