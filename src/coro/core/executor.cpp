/**
 * @file executor.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <xsl/coro/core/executor.h>
#include <xsl/coro/def.h>
#include <xsl/coro/log.h>
#include <xsl/log.h>

#include <thread>
XSL_CORO_NB
void NoopExecutor::schedule(move_only_function<void()> &&func) { func(); }

void NewThreadExecutor::schedule(move_only_function<void()> &&func) {
  co_debug("new task scheduled");
  std::thread(std::move(func)).detach();
}

struct ThreadPoolExecutor::State {
  std::vector<std::thread> workers;
  std::queue<move_only_function<void()>> tasks;
  std::mutex mtx;
  std::condition_variable cv;
  bool stop = false;

  // Runs on whichever thread drops the last reference — possibly a pool
  // worker (its captured `state` being the last one). Join every worker
  // except the current thread, which cannot join itself.
  ~State() {
    {
      std::lock_guard lock(mtx);
      stop = true;
    }
    cv.notify_all();
    for (auto& w : workers) {
      if (!w.joinable()) continue;
      if (w.get_id() == std::this_thread::get_id()) {
        w.detach();
      } else {
        w.join();
      }
    }
  }
};

ThreadPoolExecutor::ThreadPoolExecutor(size_t n) : _state(std::make_shared<State>()) {
  auto state = _state;
  for (size_t i = 0; i < n; i++) {
    state->workers.emplace_back([state] {
      for (;;) {
        move_only_function<void()> task;
        {
          std::unique_lock lock(state->mtx);
          state->cv.wait(lock, [&] { return state->stop || !state->tasks.empty(); });
          if (state->stop && state->tasks.empty()) return;
          task = std::move(state->tasks.front());
          state->tasks.pop();
        }
        task();
      }
    });
  }
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
  // the handle only signals stop; workers drain remaining tasks and the
  // State is destroyed (and workers joined) by whichever thread exits last
  {
    std::lock_guard lock(_state->mtx);
    _state->stop = true;
  }
  _state->cv.notify_all();
}

void ThreadPoolExecutor::schedule(move_only_function<void()> &&func) {
  {
    std::lock_guard lock(_state->mtx);
    _state->tasks.emplace(std::move(func));
  }
  _state->cv.notify_one();
}

XSL_CORO_NE
