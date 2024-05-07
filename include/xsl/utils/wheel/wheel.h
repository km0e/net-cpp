#pragma once

#ifndef _XSL_UTILS_WHEEL_H_
#  define _XSL_UTILS_WHEEL_H_
#  include <algorithm>
#  include <array>
#  include <atomic>
#  include <concepts>
#  include <functional>
#  include <memory>
#  include <mutex>
#  include <optional>
#  include <queue>
#  include <string>
#  include <string_view>
#  include <tuple>
#  include <unordered_map>
#  include <utility>
#  include <variant>
#  include <vector>

namespace wheel {
  using std::array;
  using std::atomic_flag;
  using std::forward;
  using std::function;
  using std::make_shared;
  using std::make_unique;
  using std::move;
  using std::mutex;
  using std::optional;
  using std::pair;
  using std::queue;
  using std::same_as;
  using std::shared_ptr;
  using std::string;
  using std::string_view;
  using std::to_string;
  using std::tuple;
  using std::unique_ptr;
  using std::unordered_map;
  using std::vector;
  // using dp::thread_pool;
  // using dp::details::default_function_type;
  using std::lock_guard;

  template <typename T, typename E>
  class RefResult {
  public:
    RefResult(const std::variant<T, E>& value) : value(value) {}
    constexpr bool is_ok() const { return std::holds_alternative<T>(value); }
    constexpr bool is_err() const { return std::holds_alternative<E>(value); }
    const T& unwrap() { return std::get<T>(value); }
    const E& unwrap_err() { return std::get<E>(value); }

  private:
    const std::variant<T, E>& value;
  };
  template <typename T, typename E>
  class Result {
  public:
    Result(T value) : value(value) {}
    Result(E error) : value(error) {}
    constexpr bool is_ok() const { return std::holds_alternative<T>(value); }
    constexpr bool is_err() const { return std::holds_alternative<E>(value); }
    T unwrap() { return std::get<T>(value); }
    E unwrap_err() { return std::get<E>(value); }
    RefResult<T, E> as_ref() { return RefResult<T, E>(value); }
  private:
    std::variant<T, E> value;
  };
}  // namespace wheel

#endif  // _XSL_UTILS_WHEEL_H_
