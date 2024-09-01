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
#  include "xsl/sync.h"

#  include <concepts>
#  include <cstddef>
#  include <memory>
#  include <optional>
#  include <ranges>
#  include <span>
#  include <unordered_map>
#  include <vector>

XSL_CORO_NB

template <class T, std::size_t MaxSignals = std::dynamic_extent>
using PubSubStorage = ShardRes<std::unordered_map<T, SignalSender<MaxSignals>>>;

template <class T, std::size_t MaxSignals = std::dynamic_extent>
class PubSub {
public:
  using value_type = T;
  using storage_type = PubSubStorage<value_type, MaxSignals>;

private:
  std::unique_ptr<storage_type> _storage;

public:
  PubSub() : _storage(std::make_unique<storage_type>()) {}
  PubSub(PubSub &&) = default;
  PubSub &operator=(PubSub &&) = default;
  ~PubSub() {
    if (this->_storage) {
      this->stop();
    }
  }
  /**
   * @brief Subscribe to the pubsub
   *
   * @tparam Args the arguments for the subscriber
   * @param args the arguments for the subscriber
   * @return std::optional<SignalReceiver<>>
   */
  template <class... Args>
  std::optional<SignalReceiver<>> subscribe(Args &&...args) {
    auto [r, w] = signal<MaxSignals>();
    auto [iter, ok]
        = this->_storage->lock()->try_emplace({std::forward<Args>(args)...}, std::move(w));
    return ok ? std::make_optional(std::move(r)) : std::nullopt;
  }
  /**
   * @brief Publish to the receiver
   *
   * @tparam _Args the arguments for the publisher
   * @param args the arguments for the publisher
   * @return requires std::constructible_from<value_type, _Args...>
   */
  template <class... _Args>
    requires std::constructible_from<value_type, _Args...>
  bool publish(_Args &&...args) {
    auto tx = [this](auto &&...args) -> auto {
      auto storage = this->_storage->lock();
      auto iter = storage->find({std::forward<_Args>(args)...});
      return iter != storage->end() ? iter->second.pin() : nullptr;
    }(std::forward<_Args>(args)...);
    if (tx) {
      tx.release();
      return true;
    }
    return false;
  }
  /**
   * @brief Publish to the receiver with a predicate
   *
   * @tparam Pred
   * @param pred the predicate
   * @return true
   * @return false
   */
  template <std::predicate<const value_type &> Pred>
  bool publish(Pred &&pred) {
    auto tx_set = *this->_storage->lock_shared()
                  | std::views::filter([&pred](auto &pair) { return pred(pair.first); })
                  | std::views::transform([](auto &pair) { return pair.second.pin(); })
                  | std::ranges::to<std::vector>();
    if (tx_set.empty()) {
      return false;
    }
    for (auto &tx : tx_set) {
      tx.release();
    }
    return true;
  }
  /// @brief Stop the pubsub
  void stop() {
    auto storage = this->_storage->lock();
    for (auto &[_, tx] : *storage) {
      tx.stop();
    }
    storage->clear();
  }
};
XSL_CORO_NE
#endif
