#pragma once

#ifndef _XSL_UTILS_WHEEL_GIANT_H_
#  define _XSL_UTILS_WHEEL_GIANT_H_
#  include <algorithm>
#  include <array>
#  include <atomic>
#  include <concepts>
#  include <cstdint>
#  include <format>
#  include <forward_list>
#  include <functional>
#  include <list>
#  include <memory>
#  include <numeric>
#  include <optional>
#  include <queue>
#  include <string>
#  include <string_view>
#  include <tuple>
#  include <unordered_map>
#  include <utility>
#  include <variant>
#  include <vector>

namespace xsl::wheel::giant {
  using std::accumulate;
  using std::array;
  using std::atomic_flag;
  using std::bind;
  using std::convertible_to;
  using std::derived_from;
  using std::format;
  using std::forward;
  using std::forward_list;
  using std::function;
  using std::get;
  using std::holds_alternative;
  using std::list;
  using std::make_optional;
  using std::make_shared;
  using std::make_unique;
  using std::move;
  using std::move_constructible;
  using std::nullopt;
  using std::optional;
  using std::pair;
  using std::queue;
  using std::runtime_error;
  using std::same_as;
  using std::shared_ptr;
  using std::string;
  using std::string_view;
  using std::to_string;
  using std::tuple;
  using std::uint8_t;
  using std::unique_ptr;
  using std::unordered_map;
  using std::variant;

  using std::vector;
  // using dp::thread_pool;
  // using dp::details::default_function_type;
  using std::lock_guard;
}  // namespace xsl::wheel::giant
#endif
