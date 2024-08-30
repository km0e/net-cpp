/**
 * @file pub_sub.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Publish-Subscribe pattern for coroutines
 * @version 0.2
 * @date 2024-08-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_PUB_SUB
#  define XSL_CORO_PUB_SUB
#  include "xsl/coro/def.h"
#  include "xsl/coro/signal.h"
#  include "xsl/logctl.h"

#  include <cstddef>
#  include <mutex>
#  include <optional>
#  include <shared_mutex>
#  include <unordered_map>

XSL_CORO_NB

template <class T, std::size_t MaxSubscribers = 1, std::size_t MaxSignals = 1>
class PubSub {
public:
  using value_type = T;
  using sender_type = SignalSender<MaxSignals>;

private:
  std::shared_mutex _mtx;
  std::unordered_map<value_type, sender_type> _subscribers;

public:
  PubSub() : _mtx(), _subscribers() {}
  ~PubSub() { this->stop(); }

  template <class... Args>
  std::optional<SignalReceiver<>> subscribe(Args &&...args) {
    std::unique_lock lock{this->_mtx};
    auto [r, w] = signal();
    auto [iter, ok] = this->_subscribers.try_emplace({std::forward<Args>(args)...}, std::move(w));
    DEBUG("{}", ok);
    return ok ? std::make_optional(std::move(r)) : std::nullopt;
  }

  template <class... Args>
  bool publish(Args &&...args) {
    auto tx = [this](Args &&...args) -> auto {
      std::shared_lock lock{this->_mtx};
      auto iter = this->_subscribers.find({std::forward<Args>(args)...});
      return iter != this->_subscribers.end() ? iter->second.pin() : nullptr;
    }(std::forward<Args>(args)...);
    if (tx) {
      tx.release();
      return true;
    }
    return false;
  }

  void stop() {
    std::lock_guard lock{this->_mtx};
    for (auto &[_, tx] : this->_subscribers) {
      tx.stop();
    }
    this->_subscribers.clear();
  }
};
XSL_CORO_NE
#endif
