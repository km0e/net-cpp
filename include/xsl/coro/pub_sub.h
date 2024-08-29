/**
 * @file pub_sub.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Publish-Subscribe pattern for coroutines
 * @version 0.1
 * @date 2024-08-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_PUB_SUB
#  define XSL_CORO_PUB_SUB
#  include "xsl/coro/def.h"
#  include "xsl/wheel.h"
#  include "xsl/logctl.h"

#  include <coroutine>
#  include <cstddef>
#  include <functional>
#  include <memory>
#  include <mutex>
#  include <optional>
#  include <shared_mutex>
#  include <unordered_map>
#  include <variant>

XSL_CORO_NB

using PubSubState = std::variant<std::size_t, std::function<void()>>;

struct PubSubEndpoint {
  PubSubEndpoint() : mtx(), state(std::size_t{0}) {}
  std::mutex mtx;
  PubSubState state;
};

template <class T>
class Subscriber {
public:
  using value_type = T;

private:
  std::shared_ptr<PubSubEndpoint> _storage;

public:
  Subscriber(std::shared_ptr<PubSubEndpoint> storage) : _storage(storage) {}

  bool await_ready() noexcept {
    this->_storage->mtx.lock();
    return std::holds_alternative<std::size_t>(this->_storage->state)
           && std::get<std::size_t>(this->_storage->state) > 0;
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    this->_storage->state = [handle] { handle.promise().resume(handle); };
    this->_storage->mtx.unlock();
  }
  bool await_resume() {
    Defer defer{[this] { this->_storage->mtx.unlock(); }};
    auto &state = std::get<std::size_t>(this->_storage->state);
    if (state > 0) {
      state--;
      return true;
    }
    return false;
  }

  operator bool() const { return this->_storage.use_count() > 0; }
};

template <class T, std::ptrdiff_t MaxSubscribers = 1>
class PubSub {
public:
  using value_type = T;
private:
  std::shared_mutex _mtx;
  std::unordered_map<value_type, std::shared_ptr<PubSubEndpoint>> _subscribers;

public:

  PubSub() : _mtx(), _subscribers() {}

  template <class... Args>
  std::optional<Subscriber<value_type>> subscribe(Args &&...args) {
    std::unique_lock lock{this->_mtx};
    auto [iter, ok] = this->_subscribers.try_emplace({std::forward<Args>(args)...},
                                                     std::make_shared<PubSubEndpoint>());
    DEBUG("{}", ok);
    return ok ? std::make_optional<Subscriber<value_type>>(iter->second) : std::nullopt;
  }

  template <class... Args>
  bool publish(Args &&...args) {
    auto subscriber = [this](Args &&...args) -> PubSubEndpoint * {
      std::shared_lock lock{this->_mtx};
      auto iter = this->_subscribers.find({std::forward<Args>(args)...});
      return iter != this->_subscribers.end() ? iter->second.get() : nullptr;
    }(std::forward<Args>(args)...);
    if (subscriber) {
      subscriber->mtx.lock();
      if (std::holds_alternative<std::size_t>(subscriber->state)) {
        auto &state = std::get<std::size_t>(subscriber->state);
        state = [state] {
          if constexpr (MaxSubscribers > 1) {
            return std::min(state + 1, MaxSubscribers);
          } else {
            return 1;
          }
        }();
        subscriber->mtx.unlock();
      } else {
        std::get<std::function<void()>>(std::exchange(subscriber->state, std::size_t{1}))();
      }
      return true;
    }
    return false;
  }

  void clear() {
    std::lock_guard lock{this->_mtx};
    for (auto &[_, state] : this->_subscribers) {
      state->mtx.lock();
      if (std::holds_alternative<std::size_t>(state->state)) {
        std::get<std::size_t>(state->state) = 0;
        state->mtx.unlock();
      } else {
        std::get<std::function<void()>>(std::exchange(state->state, std::size_t{0}))();
      }
    }
    this->_subscribers.clear();
  }
};
XSL_CORO_NE
#endif
