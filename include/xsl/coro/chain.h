/**
 * @file chain.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_CHAIN
#  define XSL_CORO_CHAIN
#  include "xsl/coro/def.h"

#  include <concepts>
#  include <coroutine>
#  include <cstdint>
#  include <memory>
#  include <tuple>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
template <class AwaiterType, class TransformGroup = std::tuple<>>//TODO: for then
class ChainAwaiter;

template <class AwaiterType, class Transform, class... Transforms>
class ChainAwaiter<AwaiterType, std::tuple<Transform, Transforms...>> : public AwaiterType {
private:
  using Base = AwaiterType;
  using Last = ChainAwaiter<AwaiterType, std::tuple<Transforms...>>;

public:
  using result_type = std::invoke_result_t<Transform, typename Last::result_type>;
  using executor_type = typename Base::executor_type;

  template <class _Promise>
  ChainAwaiter(std::coroutine_handle<_Promise> handle,
               std::tuple<Transform, Transforms...> &&transforms)
      : Base(handle), _transforms(std::move(transforms)) {}

  ChainAwaiter(AwaiterType &&awaiter, std::tuple<Transform, Transforms...> &&transforms)
      : Base(std::move(awaiter)), _transforms(std::move(transforms)) {}

  result_type await_resume() {
    return this->_transform<sizeof...(Transforms)>(Base::await_resume());
  }

  template <std::invocable<result_type> _Transform>
  decltype(auto) transform(this ChainAwaiter self, _Transform &&transform) {
    using Next = ChainAwaiter<AwaiterType, std::tuple<_Transform, Transform, Transforms...>>;
    // next transforms
    auto next_transforms
        = std::tuple_cat(std::make_tuple(std::forward<_Transform>(transform)), self._transforms);
    return Next{self._handle, next_transforms};
  }

  template <class E>
    requires std::constructible_from<std::shared_ptr<executor_type>, E>
  auto &&by(this auto &&self, E &&executor) {
    self._handle.promise().by(std::forward<E>(executor));
    return std::forward<decltype(self)>(self);
  }

protected:
  std::tuple<Transform, Transforms...> _transforms;

  template <uint8_t Index, class _ResultType>
  result_type _transform(_ResultType &&result) {
    if constexpr (Index == sizeof...(Transforms)) {
      return std::get<Index>(_transforms)(std::forward<_ResultType>(result));
    } else {
      return std::get<Index>(_transforms)(_transform<Index - 1>(std::forward<_ResultType>(result)));
    }
  }
};
template <class AwaiterType>
class ChainAwaiter<AwaiterType, std::tuple<>> : public AwaiterType {
private:
  using Base = AwaiterType;

public:
  using result_type = Base::result_type;

  template <class _Promise>
  ChainAwaiter(std::coroutine_handle<_Promise> handle) : Base(handle) {}

  ChainAwaiter(AwaiterType &&awaiter) : Base(std::move(awaiter)) {}

  result_type await_resume() { return Base::await_resume(); }

  template <std::invocable<result_type> _Transform>
  decltype(auto) transform(this ChainAwaiter self, _Transform &&transform) {
    using Next = ChainAwaiter<AwaiterType, std::tuple<_Transform>>;
    // next transforms
    return Next{std::exchange(self._handle, {}),
                std::make_tuple(std::forward<_Transform>(transform))};
  }
};
XSL_CORO_NE
#endif
