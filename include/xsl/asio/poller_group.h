/**
 * @file poller_group.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Run a server on N independent poller threads (SO_REUSEPORT)
 * @version 0.1.0
 * @date 2025-09-08
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_POLLER_GROUP
#  define XSL_ASIO_POLLER_GROUP
#  include <xsl/asio/def.h>
#  include <xsl/coro.h>
#  include <xsl/log.h>

#  include <thread>
#  include <utility>
#  include <vector>

XSL_ASIO_NB

/**
 * @brief run a server on N independent poller threads
 *
 * Each poller owns a fresh IOContext and a CoroContext bound to the shared
 * NoopExecutor: coroutine continuations are dispatched INLINE on the poller
 * thread that woke them, so a connection's whole request/response cycle runs
 * on one thread with zero cross-thread hops (the standalone-asio model,
 * replicated per core). The listeners bind the same port with SO_REUSEPORT
 * (enabled by default in TcpIpSocketCreator::b since the multi-poller model)
 * and the kernel load-balances connections across them.
 *
 * Ownership & invariants:
 * - each ctx (and its Rc) is used only by its own poller's coroutine chain;
 *   the group's stored Rc is only touched on the caller thread before the
 *   poller starts and after it is joined (docs/architecture.md §3.5)
 * - each IOContext is an independent cancellation domain: shutdown() cancels
 *   only that poller's connections
 * - the service handed to the setups is used CONCURRENTLY from all pollers:
 *   route/handler lookups are read-only and safe, handlers must not mutate
 *   captured shared state
 *
 * @note handler code runs inline on the poller thread: a blocking handler
 *       stalls that poller (same tradeoff as a single-thread asio
 *       io_context); use a detached task on another executor for blocking work
 */
class PollerGroup {
public:
  /// @param n the number of pollers (each one poller thread + one epoll)
  explicit PollerGroup(unsigned n) : n_(n) {}

  PollerGroup(const PollerGroup&) = delete;
  PollerGroup& operator=(const PollerGroup&) = delete;

  ~PollerGroup() {
    this->stop();
    this->join();
  }

  /**
   * @brief create the per-poller contexts, detach one setup task per poller
   *        and spawn one thread per poller running io.run()
   *
   * @param setup invoked as setup(ctx, io) and must return the server's
   *        accept task (Task<void>); it runs INLINE on the calling thread
   *        until its first await — create the listener before the first
   *        co_await so all ports are bound synchronously when start() returns
   * @return Expected<void, errc>
   */
  template <class Setup>
    requires std::invocable<Setup&, CoroContext&, sys::IOContext&>
  Expected<void, errc> start(Setup&& setup) {
    this->ctxs_.reserve(this->n_);
    for (unsigned i = 0; i < this->n_; ++i) {
      TRVEC(ctx, xsl::asio::asio_ctx(noop_executor()));
      auto* io = static_cast<sys::IOContext*>(ctx->get_reserved());
      std::move(setup(*ctx, *io)).detach(*ctx);
      this->ctxs_.push_back(std::move(ctx));
      // the thread only needs the raw IOContext: its lifetime is backed by
      // the ctx's reserved object, which outlives the thread (joined in dtor)
      this->threads_.emplace_back([io] { io->run(); });
    }
    return {};
  }

  /// @brief stop every poller (idempotent): each IOContext shuts down its own
  ///        cancellation domain and wakes all suspended IO awaits
  void stop() {
    for (auto& ctx : this->ctxs_) {
      static_cast<sys::IOContext*>(ctx->get_reserved())->shutdown();
    }
  }

  /// @brief join all poller threads (blocks until run() returns in each)
  void join() {
    if (this->joined_) return;
    for (auto& thread : this->threads_) {
      if (thread.joinable()) thread.join();
    }
    this->joined_ = true;
  }

  /// @brief the number of pollers actually created
  [[nodiscard]] unsigned size() const noexcept {
    return static_cast<unsigned>(this->ctxs_.size());
  }

private:
  unsigned n_;
  std::vector<Rc<CoroContext>> ctxs_;
  std::vector<std::thread> threads_;
  bool joined_ = false;
};

XSL_ASIO_NE
#endif
