# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Xsl is a C++26 network library using coroutines (C++20 `std::coroutine`), epoll-based I/O, and the quill logging framework. It compiles with GCC 16 / Clang, links with mold, and uses CPM.cmake for dependency management.

## Build Commands

```bash
# Configure (network needed for first-time dependency download)
fish -c 'proxy cmake -B build -S .'

# Build all
cmake --build build -j$(nproc)

# Build specific target
cmake --build build --target xsl_log

# Run all tests
ctest --test-dir build

# Run single test
ctest --test-dir build -R channel
# or directly:
./build/test/unit/coro/channel

# Run failed tests verbosely
ctest --test-dir build --rerun-failed --output-on-failure
```

`-DXSL_LOG_LEVEL=TRACE` controls compile-time log filtering (default: INFO).

## Library Architecture

### Target Dependency Graph

```
xsl_log (STATIC, quill init + log macros)
   ├── xsl_sys    (STATIC) — syscall wrappers, epoll, socket, sockaddr
   ├── xsl_wheel  (STATIC) — utility: str, bit, rc (refcount), type_traits
   ├── xsl_coro   (STATIC) — coroutine primitives: task, channel, signal, pub/sub
   ├── xsl_net    (STATIC) — DNS, HTTP protocol, URI parsing
   └── xsl_asio   (STATIC) — async I/O: HTTP client/server, TCP/UDP, pipe, TLS (OpenSSL)
         └── depends on xsl_sys + xsl_net + xsl_coro + xsl_wheel + OpenSSL

xsl (STATIC, umbrella) — aggregates all above, exposes xsl.cpp
```

- All sub-libraries link `xsl_log PUBLIC` (transitively provides include dir and quill)
- Tests link the specific sub-library they test, plus `GTest::gtest_main`
- Examples link `xsl_asio` (covers the full stack) plus `clilib` (CLI11)

### Namespace Conventions

Macro pairs that expand to nested namespace blocks:

| Macro Pair | Expands to |
|---|---|
| `XSL_NB` / `XSL_NE` | `namespace xsl {` / `}` |
| `XSL_CORO_NB` / `XSL_CORO_NE` | `namespace xsl::coro {` / `}` |
| `XSL_NET_NB` / `XSL_NET_NE` | `namespace xsl::_net {` / `}` |
| `XSL_SYS_NET_NB` / `XSL_SYS_NET_NE` | `namespace xsl::sys::net {` / `}` |

Files follow: `XSL_{MODULE}_NB` / `XSL_{MODULE}_NE` around the body, consistent `ifndef` header guard matching the file path.

## Source Layout

```
include/xsl/          — Public headers, mirror src/ structure
  log.h               — Log macros, LogCtl class, logger_xsl global
  error.h             — Expected<T>, ResultError<T>, TRV/ENSURE/MUST macros, errc formatter
  def.h               — XSL_NB/NE macros, using declarations (errc, expected, optional)
  concept.h           — C++20 concepts
  compose.h           — shared_memory<T> (atomic refcount, chainable ->) + LocalComosite (direct Part inheritance)
  io.h, io/           — Buffer, I/O context types
  coro/               — Coroutine headers (task, channel, signal, pub_sub)
  net/                — DNS, HTTP protocol types
  sys/                — epoll, socket, sockaddr wrappers
  asio/               — Async I/O (HTTP, TCP, TLS, pipe) — uses compose2.h
  wheel/              — Utilities (str, bit, rc, type_traits)
  sync/               — SPSC queue
src/                  — Implementation, same structure as include/xsl/
test/unit/            — Unit tests per module, each test is a single executable
examples/             — Standalone example executables
cmake/
  deps.cmake          — CPMAddPackage declarations (quill, CLI11, googletest)
  CPM.cmake           — CPM.cmake itself (vendored)
```

### Compose2 Architecture

Async types use `shared_memory<T>` + `LocalComosite2<S...>`:

```
shared_memory<T>                    — shared ownership (atomic refcount)
  ├── emplace(args...)             — 原地重建（自主管理）
  ├── destroy()                    — 析构
  └── operator->() → T* | Chain    — 链式代理（T 有 operator->() 时链式）

LocalComosite2<Alloc, S...> : public S...  — 直接继承所有 Parts
  ├── emplace<Part>(args...)       — 原地重建 Part
  ├── destroy<Part>()              — 析构 Part
  └── abandon<Parts...>()          — 批量析构
```

外部调用统一用 `dev->method()`。`into_dyn()` 已废弃。详见 `docs/migration-compose2-progress.md`。

## Code Conventions

- C++26 (`std::cmake 26`): uses deducing `this` (`this auto&& self`), `std::expected`, `std::move_only_function`, `std::ranges::to`, `std::views::split`
- `#pragma once` + traditional `#ifndef XSL_{PATH}` guard (both, for tooling compatibility)
- Doxygen `@file @author @brief @version @date` comment block on every header
- Error handling pattern: `TRV(var, expr)` / `ENSURE(expr)` / `MUST(expr)` macros wrapping `xsl::Expected<T>` (≈ `std::expected<T, unique_ptr<Error>>`)
- Return types use `explicit operator bool()`, checked via the macros above
- Compile-time log filtering via `QUILL_COMPILE_ACTIVE_LOG_LEVEL` (set per-target, propagated PUBLIC)
- `log_trace/debug/info/warning/error/critical` macros expand to quill macros on `xsl::logger_xsl`

## Environment

- Linker: mold (`-fuse-ld=mold` in CMakeLists.txt)
- `CPP_LOG` env var controls runtime log level per-logger (e.g. `CPP_LOG=info` or `CPP_LOG=xsl=debug,coro=info`)
- `CPM_SOURCE_CACHE` env var caches downloaded dependencies across clean builds
