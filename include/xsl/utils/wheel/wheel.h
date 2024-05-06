#pragma once
#ifndef _XSL_UTILS_WHEEL_H_
#define _XSL_UTILS_WHEEL_H_


#include <vector>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>

namespace wheel {
  using std::string;
  using std::string_view;
  using std::unique_ptr;
  using std::make_unique;
  using std::tuple;
  using std::function;
  using std::unordered_map;
  using std::shared_ptr;
  using std::queue;
  using std::make_shared;
  using std::mutex;
  using std::atomic_flag;
  using std::to_string;
  using std::pair;
  using std::optional;
  using std::vector;
  // using dp::thread_pool;
  // using dp::details::default_function_type;
  using std::lock_guard;

  template <typename T, typename E>
  class Result {
  public:
    Result(T value) : value(value) {}
    Result(E error) : value(error) {}
    bool is_ok() const {
      return std::holds_alternative<T>(value);
    }
    bool is_err() const {
      return std::holds_alternative<E>(value);
    }
    T unwrap() {
      return std::get<T>(value);
    }
    E unwrap_err() {
      return std::get<E>(value);
    }
  private:
    std::variant<T, E> value;
  };

}  // namespace wheel

#endif  // _XSL_UTILS_WHEEL_H_
