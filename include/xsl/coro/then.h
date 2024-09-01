/**
 * @file then.h
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
#  include <tuple>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
template <class AwaiterType, class TransformGroup = std::tuple<>>
class ThenAwaiter;

template <class AwaiterType, class Transform, class... Transforms>
class ThenAwaiter<AwaiterType, std::tuple<Transform, Transforms...>> : public AwaiterType {
private:
  using Base = AwaiterType;
  using Last = ThenAwaiter<AwaiterType, std::tuple<Transforms...>>;

public:
  using result_type = std::invoke_result_t<Transform, typename Last::result_type>;
  using executor_type = typename Base::executor_type;

  template <class _Promise>
  constexpr ThenAwaiter(std::coroutine_handle<_Promise> handle,
                        std::tuple<Transform, Transforms...> &&transforms)
      : Base(handle), _transforms(std::move(transforms)) {}

  constexpr ThenAwaiter(AwaiterType &&awaiter, std::tuple<Transform, Transforms...> &&transforms)
      : Base(std::move(awaiter)), _transforms(std::move(transforms)) {}

  constexpr result_type await_resume() {
    return this->_transform<sizeof...(Transforms)>(Base::await_resume());
  }

  template <std::invocable<result_type> _Transform>
  constexpr decltype(auto) then(this ThenAwaiter self, _Transform &&transform) {
    using Next = ThenAwaiter<AwaiterType, std::tuple<_Transform, Transform, Transforms...>>;
    // next transforms
    auto next_transforms
        = std::tuple_cat(std::make_tuple(std::forward<_Transform>(transform)), self._transforms);
    return Next{self._handle, next_transforms};
  }

protected:
  std::tuple<Transform, Transforms...> _transforms;

  template <uint8_t Index>
  constexpr result_type _transform(auto &&result) {
    if constexpr (Index == sizeof...(Transforms)) {
      return std::get<Index>(_transforms)(std::forward<decltype(result)>(result));
    } else {
      return std::get<Index>(_transforms)(
          _transform<Index - 1>(std::forward<decltype(result)>(result)));
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

  constexpr ThenAwaiter(AwaiterType &&awaiter) : Base(std::move(awaiter)) {}

  constexpr result_type await_resume() { return Base::await_resume(); }

  template <std::invocable<result_type> _Transform>
  constexpr decltype(auto) then(this ThenAwaiter self, _Transform &&transform) {
    using Next = ThenAwaiter<AwaiterType, std::tuple<_Transform>>;
    // next transforms
    return Next{std::exchange(self._handle, {}),
                std::make_tuple(std::forward<_Transform>(transform))};
  }
};
XSL_CORO_NE
#endif
