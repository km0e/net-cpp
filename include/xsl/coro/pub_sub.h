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
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal.h>
#  include <xsl/feature.h>
#  include <xsl/type_traits.h>

#  include <concepts>
#  include <cstddef>
#  include <unordered_map>
#  include <utility>

XSL_CORO_NB

template <class Storage>
struct PubSubTraits;

template <class Storage>
class PubSub : public PubSubTraits<Storage> {
  using traits_type = PubSubTraits<Storage>;
  using key_type = typename traits_type::key_type;
  using signal_type = typename traits_type::signal_type;

public:
  using traits_type::traits_type;

  template <key_type key>
  constexpr signal_type *signal() {
    if constexpr (requires { this->traits_type::template signal<key>(); }) {
      return this->traits_type::template signal<key>();
    } else {
      return this->traits_type::signal(key_type{key});
    }
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
  template <key_type key>
  constexpr bool publish() {
    if (signal_type *tx = this->PubSub::signal<key>()) {
      tx->release();
      return true;
    } else {
      return false;
    }
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
  template <typename... _Args>
  constexpr bool publish(_Args &&...args) {
    key_type key{std::forward<_Args>(args)...};
    if (signal_type *tx = this->traits_type::signal(key)) {
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
  template <std::predicate<const key_type &> Pred>
  constexpr bool publish(Pred &&pred) {
    bool empty = true;
    auto f = [&empty, &pred](const key_type &key, auto &tx) {
      if (pred(key)) {
        tx.release();
        empty = false;
      }
    };
    this->for_each(f);
    return !empty;
  }
  /// @brief Stop the pubsub
  constexpr void stop() {
    auto f = [](const key_type &, auto &tx) { tx.stop(); };
    this->for_each(f);
  }
};

template <typename K, typename S, K... keys>
struct StaticExactPubSubStorage : public std::array<S, sizeof...(keys)> {};

template <typename K, typename S, K... keys>
struct PubSubTraits<StaticExactPubSubStorage<K, S, keys...>>
    : public StaticExactPubSubStorage<K, S, keys...> {
  using key_type = K;
  using signal_type = S;
  using storage_type = StaticExactPubSubStorage<K, S, keys...>;

  using storage_type::storage_type;

protected:
  template <std::invocable<K, S &> F>
  constexpr void for_each(F &&_f) {
    [this]<std::size_t... I>(std::index_sequence<I...>, F &&_f) {
      ((std::invoke(_f, keys, (*this)[I])), ...);
    }(std::make_index_sequence<(sizeof...(keys))>{}, std::forward<F>(_f));
  }

public:
  template <K key>
  constexpr auto signal() {
    const std::size_t index = find_value_v<key, _value_pack<keys...>>;
    if constexpr (index < sizeof...(keys)) {
      return &(*this)[index];
    } else {
      return nullptr;
    }
  }
};

template <class K, std::size_t N, class Signal = SPSCSignal2<>>
using ExactPubSubStorage = std::array<std::pair<K, Signal>, N>;

template <typename K, std::size_t N, typename S>
struct PubSubTraits<ExactPubSubStorage<K, N, S>> : public ExactPubSubStorage<K, N, S> {
  using key_type = K;
  using signal_type = S;
  using storage_type = ExactPubSubStorage<K, N, S>;

  using storage_type::storage_type;

  constexpr PubSubTraits(std::same_as<K> auto... keys)
      : storage_type{{std::pair{std::forward<decltype(keys)>(keys), S()}...}} {}

protected:
  template <std::invocable<K, S &> F>
  constexpr void for_each(F &&_f) {
    for (auto &[keys, s] : *this) {
      std::invoke(_f, keys, s);
    }
  }

public:
  template <K key>
  constexpr auto signal() {
    auto iter = std::find_if(this->begin(), this->end(),
                             [](const auto &pair) { return pair.first == key; });
    if (iter != this->end()) {
      return &iter->second;
    } else {
      return nullptr;
    }
  }
};

template <typename K, typename S>
struct PubSubTraits<std::unordered_map<K, S>> : public std::unordered_map<K, S> {
  using key_type = K;
  using signal_type = S;
  using storage_type = std::unordered_map<K, S>;

  constexpr PubSubTraits(std::same_as<K> auto... keys)
      : storage_type{{std::forward<decltype(keys)>(keys), S()}...} {}

protected:
  template <std::invocable<K, S &> F>
  constexpr void for_each(F &&_f) {
    for (auto &[keys, s] : *this) {
      std::invoke(_f, keys, s);
    }
  }

public:
  /**
   * @brief Subscribe to the receiver
   *
   * @param args
   * @return std::optional<SignalReceiver<>>
   */
  constexpr std::pair<S *, bool> subscribe(auto &&...args) {
    auto [iter, ok] = this->try_emplace({std::forward<decltype(args)>(args)...});
    return {&iter->second, ok};
  }

  constexpr S *signal(key_type key) {
    auto iter = this->find(key);
    if (iter != this->end()) {
      return &iter->second;
    } else {
      return nullptr;
    }
  }
};

namespace _pub_sub {
  template <class...>
  struct Create;

  template <class K, class S, class _Shared, K... keys>
  struct Create<_value_pack<keys...>, S, _Shared, Placeholder> {
    constexpr decltype(auto) operator()(auto...) {
      using PubSub = PubSub<StaticExactPubSubStorage<K, S, keys...>>;
      if constexpr (std::is_same_v<_Shared, Shared>) {
        return std::make_shared<PubSub>();
      } else {
        return PubSub{};
      }
    }
  };

  template <class K, class S, class _Shared>
  struct Create<K, S, _Shared, Placeholder> {
    constexpr decltype(auto) operator()(std::same_as<K> auto... keys) {
      using PubSub = PubSub<std::unordered_map<K, S>>;
      if constexpr (std::is_same_v<_Shared, Shared>) {
        return std::make_shared<PubSub>(keys...);
      } else {
        return PubSub{keys...};
      }
    }
  };

  template <class K, class S, class _Shared>
  struct Create<K, S, Placeholder, _Shared, Exact> {
    constexpr decltype(auto) operator()(std::same_as<K> auto... keys) {
      using PubSub = PubSub<ExactPubSubStorage<K, sizeof...(keys), S>>;
      if constexpr (std::is_same_v<_Shared, Shared>) {
        return std::make_shared<PubSub>(keys...);
      } else {
        return PubSub{keys...};
      }
    }
  };
}  // namespace _pub_sub

template <class K, class S, class... Features>
constexpr decltype(auto) make_pub_sub(std::same_as<K> auto... keys) {
  using F = organize_feature_flags_t<
      _pub_sub::Create<Item<always_true, void>, Item<always_true, void>, Shared, Exact>, K, S,
      Features...>;

  return F{}(keys...);
}

static_assert(std::is_same_v<decltype(make_pub_sub<_value_pack<(int)1, 2>, SPSCSignal2<1>>()),
                             PubSub<StaticExactPubSubStorage<int, SPSCSignal2<1>, 1, 2>>>);

XSL_CORO_NE
#endif
