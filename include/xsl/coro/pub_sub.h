/**
 * @file pub_sub.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Publish-Subscribe pattern for coroutines
 * @version 0.4.0
 * @date 2024-08-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_CORO_PUB_SUB
#  define XSL_CORO_PUB_SUB
#  include <xsl/compose.h>
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal.h>
#  include <xsl/feature.h>
#  include <xsl/type_traits.h>

#  include <concepts>
#  include <cstddef>
#  include <unordered_map>
#  include <utility>

XSL_CORO_NB

template <class K, class S, K... keys>
struct PubSubUtil {
public:
  /**
   * @brief Publish to the receiver
   *
   * @tparam _Args
   * @param args
   * @return true if the publisher is successful
   * @return false if the publisher is not successful
   * @note This function just traverses the storage and finds the receiver with the key
   */
  template <K key>
    requires(sizeof...(keys) == 0) || (find_value_v<key, _value_pack<keys...>> < sizeof...(keys))
  constexpr bool publish(this auto &&self) {
    if constexpr (sizeof...(keys) > 0) {
      if (auto tx = self.template signal<key>()) {
        tx->release();
        return true;
      }
    } else {
      if (auto tx = self.signal(key)) {
        tx->release();
        return true;
      }
    }
    return false;
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
  constexpr bool publish(this auto &&self, auto &&...args)
    requires std::constructible_from<K, decltype(args)...>
  {
    if (auto tx = self.signal(std::forward<decltype(args)>(args)...)) {
      tx->release();
      return true;
    } else {
      return false;
    }
  }
  /**
   * @brief Publish to the receiver with a predicate
   *
   * @tparam Pred
   * @param pred the predicate
   * @return true
   * @return false
   */
  constexpr bool publish(this auto &&self, std::invocable<K> auto &&pred) {
    bool empty = true;
    auto f = [&empty, &pred](const auto &key, auto &tx) {
      if (pred(key)) {
        tx.release();
        empty = false;
      }
    };
    self.for_each(f);
    return !empty;
  }
  /// @brief Stop the pubsub
  constexpr void stop(this auto &&self) {
    auto f = [](const auto &, auto &tx) { tx.stop(); };
    self.for_each(f);
  }
};

template <typename K, typename S, K... keys>
struct StaticExactPubSubStorage : public std::array<S, sizeof...(keys)> {};

template <class K, std::size_t N, class Signal = SPSCSignal2<>>
using ExactPubSubStorage = std::array<std::pair<K, Signal>, N>;

namespace _pub_sub {
  template <class...>
  struct Create;

  template <class K, class S, K... keys>
  struct Create<_value_pack<keys...>, S, Placeholder>
      : std::type_identity<StaticExactPubSubStorage<K, S, keys...>> {};

  template <class K, class S>
  struct Create<K, S, Placeholder> : std::type_identity<std::unordered_map<K, S>> {};

  template <class K, class S, std::size_t N>
  struct Create<K, S, Exact<N>> : std::type_identity<ExactPubSubStorage<K, N, S>> {};

}  // namespace _pub_sub

template <class K, class S, class... Features>
constexpr decltype(auto) make_pub_sub(std::same_as<K> auto... keys) {
  using Storage
      = select_feature_flags_t<_pub_sub::Create<Item<always_true<Placeholder, void>, void>,
                                                Item<always_true<Placeholder, void>, void>,
                                                Item<is_same_pack<Placeholder, void>, Exact<0>>>,
                               K, S, Features...>::type;
  SharedStorageCompose<Storage> ps{};
  std::construct_at<Storage>(ps.get(), keys...);
  // StorageTraits<Storage>::init(&ps.template acc<Storage>(), keys...);

  return ps;
}
XSL_CORO_NE
XSL_NB

template <typename K, typename S, K... keys>
struct StorageUtils<coro::StaticExactPubSubStorage<K, S, keys...>>
    : coro::StaticExactPubSubStorage<K, S, keys...>, coro::PubSubUtil<K, S, keys...> {
  using key_type = K;
  using signal_type = S;
  using storage_type = coro::StaticExactPubSubStorage<K, S, keys...>;

  constexpr void for_each(this auto &&self, std::invocable<K, S &> auto &&_f) {
    [&self]<std::size_t... I>(std::index_sequence<I...>, auto &&_f) {
      ((std::invoke(std::forward<decltype(_f)>(_f), keys, self[I])), ...);
    }(std::make_index_sequence<(sizeof...(keys))>{}, std::forward<decltype(_f)>(_f));
  }

  template <K key>
  constexpr auto signal(this auto &&self) {
    const std::size_t index = find_value_v<key, _value_pack<keys...>>;
    if constexpr (index < sizeof...(keys)) {
      return &self[index];
    } else {
      return nullptr;
    }
  }
};

template <typename K, std::size_t N, typename S>
struct StorageUtils<coro::ExactPubSubStorage<K, N, S>> : coro::ExactPubSubStorage<K, N, S>,
                                                         coro::PubSubUtil<K, S> {
  using key_type = K;
  using signal_type = S;
  using storage_type = coro::ExactPubSubStorage<K, N, S>;

  using storage_type::storage_type;

  constexpr StorageUtils(std::same_as<K> auto... keys)
      : storage_type{{std::pair{std::forward<decltype(keys)>(keys), S()}...}} {}

  template <std::invocable<K, S &> F>
  constexpr void for_each(F &&_f) {
    for (auto &[keys, s] : *this) {
      std::invoke(_f, keys, s);
    }
  }

  template <K key>
  constexpr auto signal(this auto &&self) {
    auto iter = std::find_if(self.begin(), self.end(),
                             [](const auto &pair) { return pair.first == key; });
    if (iter != self.end()) {
      return &iter->second;
    } else {
      return nullptr;
    }
  }
};

template <typename K, typename S>
struct StorageUtils<std::unordered_map<K, S>> : std::unordered_map<K, S>, coro::PubSubUtil<K, S> {
  using key_type = K;
  using signal_type = S;
  using storage_type = std::unordered_map<K, S>;

  constexpr StorageUtils(std::same_as<K> auto... keys)
      : storage_type{{std::forward<decltype(keys)>(keys), S()}...} {}

  template <std::invocable<K, S &> F>
  constexpr void for_each(F &&_f) {
    for (auto &[keys, s] : *this) {
      std::invoke(_f, keys, s);
    }
  }

  /**
   * @brief Subscribe to the receiver
   *
   * @param args
   * @return std::optional<SignalReceiver<>>
   */
  constexpr std::pair<S *, bool> subscribe(this auto &&self, auto &&...args) {
    auto [iter, ok] = self.try_emplace({std::forward<decltype(args)>(args)...});
    return {&iter->second, ok};
  }

  constexpr S *signal(this auto &&self, auto &&...args)
    requires std::constructible_from<K, decltype(args)...>
  {
    K key{std::forward<decltype(args)>(args)...};
    auto iter = self.find(key);
    if (iter != self.end()) {
      return &iter->second;
    } else {
      return nullptr;
    }
  }

  template <K key>
  constexpr S *signal(this auto &&self) {
    return self.signal(key);
  }
};
XSL_NE

#endif
