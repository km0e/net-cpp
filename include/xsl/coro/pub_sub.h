/**
 * @file pub_sub.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Publish-Subscribe pattern for coroutines
 * @version 0.3
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

XSL_CORO_NB

template <class Storage>
class PubSubBase {
public:
  using key_type = std::decay_t<std::tuple_element_t<0, typename Storage::value_type>>;
  using storage_type = Storage;

protected:
  std::shared_ptr<ShardRes<storage_type>> _storage;

public:
  constexpr PubSubBase(ShardRes<storage_type> *storage) : _storage(storage) {}
  constexpr PubSubBase(PubSubBase &&) = default;
  constexpr PubSubBase(const PubSubBase &) = default;
  constexpr PubSubBase &operator=(PubSubBase &&) = default;
  constexpr PubSubBase &operator=(const PubSubBase &) = default;
  constexpr ~PubSubBase() {
    if (this->_storage) {
      this->stop();
    }
  }
  /**
   * @brief Get the immediate signal
   *
   * @param key
   * @return constexpr decltype(auto)
   * @note The signal's lifetime is managed by the pubsub
   */
  constexpr decltype(auto) imm_signal(const key_type &key) {
    auto guard = this->_storage->lock_shared();
    auto iter = std::ranges::find_if(*guard, [&key](auto &pair) { return pair.first == key; });
    return iter != guard->end() ? iter->second.pin() : nullptr;
  }
  /**
   * @brief Get the signal
   *
   * @param key
   * @return constexpr decltype(auto)
   * @note The signal's lifetime is managed by the pubsub, but the signal can be safely released
   */
  constexpr decltype(auto) signal(const key_type &key) {
    auto guard = this->_storage->lock_shared();
    auto iter = std::ranges::find_if(*guard, [&key](auto &pair) { return pair.first == key; });
    return iter != guard->end() ? std::make_optional(iter->second) : std::nullopt;
  }
  /**
   * @brief Publish to the receiver
   *
   * @tparam _Args
   * @param args
   * @return true if the publisher is successful
   * @return false if the publisher is not successful
   * @note This function just traverses the storage and finds the receiver with the key
   */
  template <class... _Args>
    requires std::constructible_from<key_type, _Args...>
  constexpr bool publish(_Args &&...args) {
    key_type key{std::forward<_Args>(args)...};
    auto tx = [this, &key]() {
      auto guard = this->_storage->lock_shared();
      auto iter = std::ranges::find_if(*guard, [&key](auto &pair) { return pair.first == key; });
      return iter != guard->end() ? iter->second.pin() : nullptr;
    }();
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
  template <std::predicate<const key_type &> Pred>
  constexpr bool publish(Pred &&pred) {
    bool empty = true;
    auto tx_set = *this->_storage->lock_shared()
                  | std::views::filter([&pred](auto &pair) { return pred(pair.first); })
                  | std::views::transform([](auto &pair) { return pair.second.pin(); });
    for (auto &&tx : tx_set) {
      tx.release();
      empty = false;
    }
    return !empty;
  }
  /// @brief Stop the pubsub
  constexpr void stop() {
    auto storage = this->_storage->lock();
    for (auto &[_, tx] : *storage) {
      tx.stop();
    }
  }
};

template <class T, std::size_t N, std::size_t MaxSignals = std::dynamic_extent>
using ExactPubSubStorage = std::array<std::pair<T, Signal<MaxSignals>>, N>;

/**
 * @brief Make a pubsub with exact keys
 *
 * @tparam T the key type
 * @tparam MaxSignals the maximum number of signals
 * @param keys
 * @return decltype(auto)
 * @code {.cpp}
 * auto [pubsub, signal1, signal2] = make_exact_pub_sub<int>(1, 2);
 * auto [pubsub1, signal3, signal4] = make_exact_pub_sub<int, 2>(3, 4);
 * @endcode
 */
template <class T, std::size_t MaxSignals = std::dynamic_extent>
constexpr decltype(auto) make_exact_pub_sub(auto &&...keys) {
  const std::size_t N = sizeof...(keys);
  return []<std::size_t... I>(std::index_sequence<I...>, auto &&..._keys) {
    auto signals = std::make_tuple((static_cast<void>(I), Signal<MaxSignals>())...);
    auto copy = signals;
    return std::make_tuple(
        PubSubBase(new ShardRes<ExactPubSubStorage<T, N, MaxSignals>>{
            {std::pair{std::forward<decltype(_keys)>(_keys), std::move(std::get<I>(signals))}...}}),
        std::move(std::get<I>(copy))...);
  }(std::make_index_sequence<N>{}, std::forward<decltype(keys)>(keys)...);
}

template <class T, std::size_t N, std::size_t MaxSignals = std::dynamic_extent>
using ExactPubSub = PubSubBase<ExactPubSubStorage<T, N, MaxSignals>>;

template <class T, std::size_t MaxSignals = std::dynamic_extent>
class PubSub : public PubSubBase<std::unordered_map<T, Signal<MaxSignals>>> {
private:
  using Base = PubSubBase<std::unordered_map<T, Signal<MaxSignals>>>;
  using typename Base::key_type;
  using typename Base::storage_type;

public:
  constexpr PubSub() : Base(new ShardRes<storage_type>()) {}
  constexpr PubSub(PubSub &&) = default;
  constexpr PubSub &operator=(PubSub &&) = default;
  constexpr ~PubSub() {}
  /**
   * @brief Subscribe to the receiver
   *
   * @param args
   * @return std::optional<SignalReceiver<>>
   */
  constexpr std::optional<Signal<MaxSignals>> subscribe(auto &&...args) {
    Signal<MaxSignals> sig{};
    auto [iter, ok]
        = this->_storage->lock()->try_emplace({std::forward<decltype(args)>(args)...}, sig);
    return ok ? std::make_optional(std::move(sig)) : std::nullopt;
  }
  /**
   * @brief Publish to the receiver
   *
   * @tparam _Args
   * @param args
   * @return
   */
  template <class... _Args>
    requires std::constructible_from<key_type, _Args...>
  constexpr bool publish(_Args &&...args) {
    auto tx = [this](auto &&...args) -> auto {
      auto storage = this->_storage->lock_shared();
      auto iter = storage->find({std::forward<decltype(args)>(args)...});
      return iter != storage->end() ? iter->second.pin() : nullptr;
    }(std::forward<_Args>(args)...);
    if (tx) {
      tx.release();
      return true;
    }
    return false;
  }
  /// @brief Publish to the receiver with a predicate
  constexpr bool publish(std::predicate<const key_type &> auto &&pred) {
    return Base::publish(std::forward<decltype(pred)>(pred));
  }
};
XSL_CORO_NE
#endif
