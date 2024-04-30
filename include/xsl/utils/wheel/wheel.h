#pragma once
#ifndef _XSL_UTILS_WHEEL_H_
#define _XSL_UTILS_WHEEL_H_

#include <mutex>
#include <utility>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <functional>
#include <memory>
#include <tuple>
#include <string>
namespace wheel {
  using std::string;
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
}  // namespace wheel

#endif  // _XSL_UTILS_WHEEL_H_