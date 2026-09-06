/**
 * @file block.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Block coroutine until the awaited coroutine finishes
 * @version 0.2.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_CORO_BLOCK
#  define XSL_CORO_BLOCK
#  include <xsl/coro/core/def.h>
#  include <xsl/coro/def.h>
#  include <xsl/coro/log.h>

#  include <cassert>
#  include <coroutine>
#  include <exception>
#  include <semaphore>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
namespace _detail {
  /**
   * @brief a shell coroutine that never starts on its own; the awaited task's
   *        final suspend transfers control into it, and its body (which
   *        contains no await expression, dodging GCC coroutine temporary
   *        lifetime bugs) releases the semaphore
   */
  struct BlockShell {
    struct promise_type {
      std::binary_semaphore* sem = nullptr;

      promise_type() = default;
      // the coroutine's parameters are forwarded to the promise constructor
      template <class... A>
      promise_type(std::binary_semaphore& sem, A&...) : sem(&sem) {}

      auto get_return_object() noexcept {
        return BlockShell{std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      std::suspend_always initial_suspend() noexcept { return {}; }
      struct FinalAwaiter {
        bool await_ready() noexcept { return false; }
        // release only after the shell is fully suspended, so the blocked
        // caller can never destroy the frame while it is still executing
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          h.promise().sem->release();
        }
        void await_resume() noexcept {}
      };
      FinalAwaiter final_suspend() noexcept { return {}; }
      void return_void() noexcept {}
      void unhandled_exception() noexcept { std::terminate(); }
    };
    std::coroutine_handle<promise_type> handle{};

    explicit BlockShell(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    BlockShell(BlockShell&& other) noexcept
        : handle(std::exchange(other.handle, {})) {}
    BlockShell(const BlockShell&) = delete;
    ~BlockShell() {
      if (handle) {
        handle.destroy();
      }
    }
  };

  template <class Awaiter>
  BlockShell block_shell(std::binary_semaphore& sem, Awaiter& task) {
    // the semaphore is released by final_suspend once this shell is fully
    // suspended; this body must contain no await expression and no result
    // move (GCC's coroutine temporary lifetime handling corrupts both), the
    // caller reads the result from the task's promise after sem.acquire()
    co_return;  // required: without it this would not be a coroutine at all
  }
}  // namespace _detail

/**
 * @brief Block the current thread until the awaited task finishes
 *
 * The task is driven manually: its awaiter is resumed from this thread and,
 * once it completes, control symmetrically transfers into a shell coroutine
 * that releases the semaphore. The shell contains no await expression, which
 * keeps GCC's coroutine temporary handling out of the picture.
 *
 * @note exceptions thrown during the task's first (inline) run are rethrown
 *       in the calling thread; exceptions after the task was resumed by
 *       another thread terminate the program (thread-per-dispatch cannot
 *       route them back)
 */
template <class Awaiter>
inline decltype(auto) block(Awaiter&& awaiter) {
  using awaiter_type = std::remove_reference_t<Awaiter>;
  using result_type = typename awaiter_traits<awaiter_type>::result_type;
  // the task must be owned by THIS frame: the shell reads it after the
  // transfer, and this frame outlives that (it returns only after
  // sem.acquire())
  awaiter_type task = std::forward<Awaiter>(awaiter);
  std::binary_semaphore sem{0};
  std::exception_ptr eptr;
  _detail::BlockShell shell = _detail::block_shell(sem, task);
  try {
    task.await_suspend(shell.handle).resume();
  } catch (...) {
    // the task failed during its first (inline) run; it never reached its
    // final suspend, so the shell never ran and the semaphore stays unreleased
    eptr = std::current_exception();
  }
  if (!eptr) {
    sem.acquire();
  }
  if (eptr) {
    std::rethrow_exception(eptr);
  }
  // the task is suspended at its final suspend point here, reading the
  // result is safe; this plain call (not a co_await) avoids the GCC
  // coroutine temporary lifetime bug
  return task.await_resume();
}

XSL_CORO_NE
#endif  // XSL_CORO_BLOCK
