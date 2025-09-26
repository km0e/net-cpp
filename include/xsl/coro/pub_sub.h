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
  constexpr bool publish(this auto&& self) {
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
  constexpr bool publish(this auto&& self, auto&&... args)
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
  constexpr bool publish(this auto&& self, std::invocable<K> auto&& pred) {
    bool empty = true;
    auto f = [&empty, &pred](const auto& key, auto& tx) {
      if (pred(key)) {
        tx.release();
        empty = false;
      }
    };
    self.for_each(f);
    return !empty;
  }
  // Stop disabled: SPSCSignal4 has no stop()
  // constexpr void stop(this auto&& self) {
  //   auto f = [](const auto&, auto& tx) { tx.stop(); };
  //   self.for_each(f);
  // }
};

template <typename K, typename S, K... keys>
struct StaticExactPubSubStorage : public std::array<S, sizeof...(keys)>,
                                  public coro::PubSubUtil<K, S, keys...> {
  using key_type = K;
  using signal_type = S;

  constexpr void for_each(this auto&& self, std::invocable<K, S&> auto&& _f) {
    [&self]<std::size_t... I>(std::index_sequence<I...>, auto&& _f) {
      ((std::invoke(std::forward<decltype(_f)>(_f), keys, self[I])), ...);
    }(std::make_index_sequence<(sizeof...(keys))>{}, std::forward<decltype(_f)>(_f));
  }

  template <K key>
  constexpr auto signal(this auto&& self) {
    const std::size_t index = find_value_v<key, _value_pack<keys...>>;
    if constexpr (index < sizeof...(keys)) {
      return &self[index];
    } else {
      return nullptr;
    }
  }
};

template <class K, std::size_t N, class Signal = SPSCSignal4>
struct ExactPubSubStorage : public std::array<std::pair<K, Signal>, N>,
                            public coro::PubSubUtil<K, Signal> {
  using key_type = K;
  using signal_type = Signal;
  using storage_type = std::array<std::pair<K, Signal>, N>;

  using storage_type::storage_type;

  constexpr ExactPubSubStorage(std::same_as<K> auto... keys)
      : storage_type{{std::pair{std::forward<decltype(keys)>(keys), Signal()}...}} {}

  template <std::invocable<K, Signal&> F>
  constexpr void for_each(F&& _f) {
    for (auto& [key, s] : *this) {
      std::invoke(_f, key, s);
    }
  }

  template <K key>
  constexpr auto signal(this auto&& self) {
    auto iter = std::find_if(self.begin(), self.end(),
                             [](const auto& pair) { return pair.first == key; });
    if (iter != self.end()) {
      return &iter->second;
    } else {
      return nullptr;
    }
  }
};

namespace _detail {
  template <class...>
  struct Create;

  template <class K, class S, K... keys>
  struct Create<_value_pack<keys...>, S, Placeholder>
      : std::type_identity<StaticExactPubSubStorage<K, S, keys...>> {};

  template <class K, class S, std::size_t N>
  struct Create<K, S, Exact<N>> : std::type_identity<ExactPubSubStorage<K, N, S>> {};

}  // namespace _detail

template <class K, class S, class... Features>
constexpr decltype(auto) make_pub_sub(std::same_as<K> auto... keys) {
  using Storage
      = select_feature_flags_t<_detail::Create<Item<always_true<Placeholder, void>, void>,
                                               Item<always_true<Placeholder, void>, void>,
                                               Item<is_same_pack<Placeholder, void>, Exact<0>>>,
                               K, S, Features...>::type;
  shared_memory<Storage> ps{};
  ps.emplace(keys...);
  return ps;
}
XSL_CORO_NE

#endif
