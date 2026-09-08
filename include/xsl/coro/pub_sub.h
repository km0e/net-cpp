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
    requires(find_value_v<key, _value_pack<keys...>> < sizeof...(keys))
  constexpr bool publish(this auto&& self) {
    if (auto tx = self.template signal<key>()) {
      tx->release();
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
  /**
   * @brief Stop all signals (sticky): suspended consumers wake with false,
   *        subsequent awaits observe the stop immediately
   */
  constexpr void stop(this auto&& self) {
    auto f = [](const auto&, auto& tx) { tx.stop(); };
    self.for_each(f);
  }
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

// NOTE: a runtime-keyed variant (ExactPubSubStorage) was removed — it was
// unused, its variadic constructor could not even compile with non-movable
// signals, and its compile-time signal<key>() on runtime storage was a
// mismatch. If runtime keys are ever needed, build a proper runtime map.

namespace _detail {
  template <class...>
  struct Create;

  template <class K, class S, K... keys>
  struct Create<_value_pack<keys...>, S, Placeholder>
      : std::type_identity<StaticExactPubSubStorage<K, S, keys...>> {};

}  // namespace _detail

template <class K, class S, class... Features>
constexpr decltype(auto) make_pub_sub(std::same_as<K> auto... keys) {
  using Storage
      = select_feature_flags_t<_detail::Create<Item<always_true<Placeholder, void>, void>,
                                               Item<always_true<Placeholder, void>, void>,
                                               // filler: consumes no flag, contributes the
                                               // Placeholder default as the 3rd Create arg
                                               Item<std::is_same<Placeholder, void>, void>>,
                               K, S, Features...>::type;
  shared_memory<Storage> ps{};
  ps.emplace(keys...);
  return ps;
}
XSL_CORO_NE

#endif
