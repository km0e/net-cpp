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

ThreadPoolExecutor::ThreadPoolExecutor(size_t n) {
  for (size_t i = 0; i < n; i++) {
    _workers.emplace_back([this] {
      for (;;) {
        move_only_function<void()> task;
        {
          std::unique_lock lock(_mtx);
          _cv.wait(lock, [this] { return _stop || !_tasks.empty(); });
          if (_stop && _tasks.empty()) return;
          task = std::move(_tasks.front());
          _tasks.pop();
        }
        task();
      }
    });
  }
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
  {
    std::lock_guard lock(_mtx);
    _stop = true;
  }
  _cv.notify_all();
  for (auto& w : _workers) {
    if (w.joinable()) w.join();
  }
}

void ThreadPoolExecutor::schedule(move_only_function<void()> &&func) {
  {
    std::lock_guard lock(_mtx);
    _tasks.emplace(std::move(func));
  }
  _cv.notify_one();
}

XSL_CORO_NE
