# AGENTS.md

This file provides guidance to coding agents (and humans) working in this repository.

## Project Overview

Xsl is a C++26 network library using coroutines (C++20 `std::coroutine`), epoll-based I/O, and the quill logging framework. It compiles with GCC 16 / Clang, links with mold, and uses CPM.cmake for dependency management.

## Build Commands

```bash
# Configure via preset (build/debug, CPM cache under ~/.cache/CPM)
cmake --preset debug
# Or plain configure (defaults to Debug when CMAKE_BUILD_TYPE is unset)
fish -c 'proxy cmake -B build -S .'

# Build all
cmake --build build -j$(nproc)        # or: cmake --build --preset debug

# Build specific target
cmake --build build --target xsl_log

# Run all tests
cmake --build build -j$(nproc) && ctest --test-dir build

# Run single test
cmake --build build --target channel && ctest --test-dir build -R channel
# or directly:
./build/test/unit/coro/channel

# Run failed tests verbosely
ctest --test-dir build --rerun-failed --output-on-failure

# Filter by label (labels = path under test/, e.g. unit, unit/coro, integration)
ctest --test-dir build -L wheel
```

Presets (`debug` / `release` / `asan` / `coverage`) live in `CMakePresets.json`.

Notable options: `-DXSL_LOG_LEVEL=` (compile-time log filtering, default INFO),
`-DXSL_BUILD_EXAMPLES=OFF`, `-DXSL_BUILD_TESTS=OFF`, `-DXSL_UNITY_BUILD=ON`
(unity build: ~2x faster full rebuilds, worse incrementals), `-DXSL_PCH=ON`,
`-DXSL_SANITIZE=address;undefined`.

Xmake is used for benchmarks/examples (release + logs compiled out):

```bash
xmake f -m release -c --log_level=none
xmake build -g benchmarks/http
xmake run --workdir=build bench_http_xsl -p 18080 -n 4
xmake run --workdir=build bench_http_loadgen -p 18080 -t 4 -c 4 -d 10
```

Package versions are pinned to match CMake (quill 10.0.1, gtest 1.17.0 with
`main = true`). Tests are registered per-source-file with meaningful names:

```bash
xmake test                    # all tests (currently 32)
xmake test --group=coro      # group = module (coro, http, wheel, integration, ...)
xmake test ut_channel        # single test by test name
```

`-n N` on `bench_http_xsl` selects the server threading model: `0` = thread-pool
executor (default), `N` = N inline pollers (SO_REUSEPORT, see
`include/xsl/asio/poller_group.h` and `docs/architecture.md` §3.2).

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

Deep dives: `docs/architecture.md` (coroutine runtime / IO / threading model,
ownership invariants) and `docs/benches/http.md` (HTTP server performance,
optimization history). The recommended HTTP server shape is the multi-poller
`PollerGroup` — see `docs/architecture.md` §3.2.

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
  compose.h           — shared_memory<T> (atomic refcount, chainable ->) + LocalComposite (direct Part inheritance)
  io.h, io/           — Buffer, I/O context types
  coro/               — Coroutine headers (task, channel, signal, pub_sub)
  net/                — DNS, HTTP protocol types
  sys/                — epoll, socket, sockaddr wrappers
  asio/               — Async I/O (HTTP, TCP, TLS, pipe, poller_group.h) — uses compose2.h
  wheel/              — Utilities (str, bit, rc, type_traits)
  sync/               — SPSC queue
src/                  — Implementation, same structure as include/xsl/
test/unit/            — Unit tests per module, each test is a single executable
test/integration/     — Cross-module integration tests (http_compare, asio bind/connect)
test/benches/         — Benchmark harnesses (http: bench.sh + loadgen + two servers)
test/include/         — Shared test helpers (http_bench/ server implementations)
examples/             — Standalone example executables
local/                — Untracked local notes (gitignored)
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

外部调用统一用 `dev->method()`。`into_dyn()` 已废弃。详见 `docs/migration-compose2-progress.md`（迁移历史与未完成项）、`docs/architecture.md`（协程运行时/IO/线程模型总览）。

## Code Conventions

- C++26 (`set_languages("cxx26")`): uses deducing `this` (`this auto&& self`), `std::expected`, `std::move_only_function`, `std::ranges::to`, `std::views::split`
- `#pragma once` + traditional `#ifndef XSL_{PATH}` guard (both, for tooling compatibility)
- Doxygen `@file @author @brief @version @date` comment block on every header
- Error handling pattern: `TRV(var, expr)` / `ENSURE(expr)` / `MUST(expr)` macros wrapping `xsl::Expected<T>` (≈ `std::expected<T, unique_ptr<Error>>`)
- Return types use `explicit operator bool()`, checked via the macros above
- Compile-time log filtering via `QUILL_COMPILE_ACTIVE_LOG_LEVEL` (set per-target, propagated PUBLIC)
- `log_trace/debug/info/warning/error/critical` macros expand to quill macros on `xsl::logger_xsl`

## Environment

- Linker: mold (`-fuse-ld=mold` in CMakeLists.txt / root xmake.lua)
- `CPP_LOG` env var controls runtime log level per-logger (e.g. `CPP_LOG=info` or `CPP_LOG=xsl=debug,coro=info`)
- `CPM_SOURCE_CACHE` env var caches downloaded dependencies across clean builds (injected by
  CMakePresets automatically; for plain `cmake -B build` runs set `export CPM_SOURCE_CACHE=~/.cache/CPM`)
- `ccache` is auto-detected by CMake and used as compiler launcher; xmake manages its own cache.
  Install it (`sudo apt install ccache`) for faster rebuilds
- Non-interactive sudo is not available in this environment

## Install

`cmake --install <build> --prefix <dir>` installs headers, the six static libs and CMake
package config (`find_package(xsl)` → `xsl::xsl`); quill is installed alongside
(`QUILL_ENABLE_INSTALL=ON`) and resolved via `find_dependency`.

## Xmake specifics

- `xmake f --unity=y` enables unity (jumbo) builds for the six library targets
  (`xsl_enable_unity()`; CMake equivalent: `-DXSL_UNITY_BUILD=ON`) — ~2x faster clean builds
- `xmake f --log_level=<level>` is the compile-time log filter (CMake: `-DXSL_LOG_LEVEL=`);
  both default to INFO, `none`/`OFF` compiles all log statements out
