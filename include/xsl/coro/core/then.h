/**
 * @file then.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief ThenAwaiter — chainable coroutine transform, O(1) template instantiation
 * @version 0.2.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 */
#pragma once
#ifndef XSL_CORO_CHAIN
#  define XSL_CORO_CHAIN
#  include <xsl/coro/def.h>
#  include <xsl/type_traits.h>

#  include <concepts>
#  include <coroutine>
#  include <tuple>
#  include <type_traits>
#  include <utility>

XSL_CORO_NB

namespace _detail {
  // Right-fold: T0(T1(T2(...AwaiterResult))) — matches old recursive design
  template <class AwaiterResult, class Tuple>
  struct compose_result;

  template <class AwaiterResult>
  struct compose_result<AwaiterResult, std::tuple<>> {
    using type = AwaiterResult;
  };

  template <class AwaiterResult, class First, class... Rest>
  struct compose_result<AwaiterResult, std::tuple<First, Rest...>> {
    using type
        = std::invoke_result_t<First,
                               typename compose_result<AwaiterResult, std::tuple<Rest...>>::type>;
  };
}  // namespace _detail

template <class AwaiterType, class TransformTuple = std::tuple<>>
class ThenAwaiter;

template <class AwaiterType, class Transform, class... Transforms>
class ThenAwaiter<AwaiterType, std::tuple<Transform, Transforms...>> : public AwaiterType {
private:
  using Base = AwaiterType;
  using TransformsTuple = std::tuple<Transform, Transforms...>;

public:
  // result_type: compose all transforms: f1 ∘ f2 ∘ ... ∘ fn (AwaiterType::result_type)
  using result_type = typename _detail::compose_result<typename AwaiterType::result_type,
                                                       std::tuple<Transform, Transforms...>>::type;

  template <class _Promise>
  constexpr ThenAwaiter(std::coroutine_handle<_Promise> handle,
                        std::tuple<Transform, Transforms...>&& transforms)
      : Base(handle), _transforms(std::move(transforms)) {}

  constexpr ThenAwaiter(AwaiterType&& awaiter, std::tuple<Transform, Transforms...>&& transforms)
      : Base(std::move(awaiter)), _transforms(std::move(transforms)) {}

  // Apply transforms left-to-right using compile-time indexed recursion (not O(n) inheritance)
  constexpr result_type await_resume() { return _apply<0>(Base::await_resume()); }

  template <std::invocable<result_type> NextTransform>
  constexpr decltype(auto) then(this ThenAwaiter&& self, NextTransform&& next) {
    using NextTuple = std::tuple<std::remove_cvref_t<NextTransform>, Transform, Transforms...>;
    auto next_transforms = std::tuple_cat(std::make_tuple(std::forward<NextTransform>(next)),
                                          std::move(self._transforms));
    return ThenAwaiter<AwaiterType, NextTuple>{std::exchange(self._handle, {}),
                                               std::move(next_transforms)};
  }

private:
  TransformsTuple _transforms;

  template <std::size_t I>
  constexpr auto _apply(auto&& value) {
    if constexpr (I == 1 + sizeof...(Transforms)) {
      return std::forward<decltype(value)>(value);
    } else {
      return std::get<I>(_transforms)(_apply<I + 1>(std::forward<decltype(value)>(value)));
    }
  }
};

template <class AwaiterType>
class ThenAwaiter<AwaiterType, std::tuple<>> : public AwaiterType {
private:
  using Base = AwaiterType;

public:
  using result_type = Base::result_type;

  template <class _Promise>
  constexpr ThenAwaiter(std::coroutine_handle<_Promise> handle) : Base(handle) {}
  constexpr ThenAwaiter(AwaiterType&& awaiter) : Base(std::move(awaiter)) {}

  constexpr result_type await_resume() { return Base::await_resume(); }

  template <std::invocable<result_type> NextTransform>
  constexpr decltype(auto) then(this ThenAwaiter&& self, NextTransform&& next) {
    return ThenAwaiter<AwaiterType, std::tuple<std::remove_cvref_t<NextTransform>>>{
        std::exchange(self._handle, {}), std::make_tuple(std::forward<NextTransform>(next))};
  }
};

XSL_CORO_NE
#endif
