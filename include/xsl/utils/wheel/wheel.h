#pragma once
#ifndef _XSL_UTILS_WHEEL_H_
#  define _XSL_UTILS_WHEEL_H_

#  include <atomic>
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
  using std::atomic_flag;
  using std::function;
  using std::make_shared;
  using std::make_unique;
  using std::mutex;
  using std::optional;
  using std::pair;
  using std::queue;
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

  template <typename T, typename E> class Result {
  public:
    Result(T value) : value(value) {}
    Result(E error) : value(error) {}
    bool is_ok() const { return std::holds_alternative<T>(value); }
    bool is_err() const { return std::holds_alternative<E>(value); }
    T unwrap() { return std::get<T>(value); }
    E unwrap_err() { return std::get<E>(value); }

  private:
    std::variant<T, E> value;
  };

}  // namespace wheel

#endif  // _XSL_UTILS_WHEEL_H_
