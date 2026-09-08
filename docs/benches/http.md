# HTTP server benchmarks: xsl::asio vs standalone asio {#bench_http}

## targets

- [x] **xsl::asio::HttpServer** — the coroutine-based HTTP server of this
      library (`xsl_asio`)
- [x] **standalone asio** — a minimal HTTP/1.1 server built directly on
      `asio::awaitable` coroutines, representing the "hand-tuned reference"

Both servers expose the same API surface:

| route        | response                                  |
|--------------|-------------------------------------------|
| `GET /hello` | `200` + `"Hello, World!"` (text/plain)    |
| other        | `404`                                     |

and both are driven by the **same load generator**
(`test/benches/http/loadgen.cpp`: keep-alive HTTP/1.1, configurable
threads/connections, per-request latency histogram, RPS + p50/p90/p99/p999/max),
so the comparison is apples-to-apples.

## layout

```
test/benches/http/
├── bench.sh           # orchestrates build + run + summary
├── loadgen.cpp        # shared load generator (POSIX, no dependencies)
├── server_asio.cpp    # standalone asio server entry
├── server_xsl.cpp     # xsl::asio server entry
└── xmake.lua
test/include/http_bench/
├── asio_server.h      # standalone asio server implementation
├── common.h           # shared route/payload contract
└── xsl_server.h       # xsl::asio server implementation
```

Behavioral equivalence with the asio reference is asserted by the
`it_http_compare` integration test (see correctness notes at the bottom);
request-reader invariants are pinned by `test/unit/http/message.cpp` and
`test/unit/http/header_map.cpp`.

## command

```bash
test/benches/http/bench.sh [duration] [threads] [conns-per-thread]
# e.g.
test/benches/http/bench.sh 10 4 4
```

The script builds in release mode with logs compiled out
(`xmake f -m release -c --log_level=none`), runs each server on a private
port, performs a warmup run, then a measured run, and prints a side-by-side
summary. Standalone steps:

```bash
xmake f -m release -c --log_level=none
xmake build -g benchmarks/http
xmake run --workdir=build bench_http_xsl -p 18080
xmake run --workdir=build bench_http_loadgen -p 18080 -t 4 -c 4 -d 10
```

## results

Machine: 16-core Linux, GCC 16, release + LTO, loopback, keep-alive.
`GET /hello`, zero errors in all runs. `bench_http_xsl -n N` selects the
server model (`0` = thread-pool executor, `N` = N inline pollers).

### current (thread-pool vs multi-poller vs standalone asio)

| scenario                    | thread-pool (`-n 0`) | pollers=8 (`-n 8`) | standalone asio    |
|-----------------------------|----------------------|--------------------|--------------------|
| 1 thread × 1 conn, RPS      | ~142,000             | ~200,000           | ~218,000           |
| 1 thread × 1 conn, p50      | 6 µs                 | **4 µs**           | 4 µs               |
| 4 threads × 4 conns, RPS    | ~566,000             | ~622,000           | ~424,000           |
| 8 threads × 8 conns, RPS    | ~998,000             | ~981,000           | ~414,000 (satur.)  |
| 16 threads × 16 conns, RPS  | ~1,122,000           | **~1,211,000**     | ~409,000 (satur.)  |
| 8×8 / 16×16 p50             | 7 µs / 13 µs         | 7 µs / 11 µs       | 18 µs / 38 µs      |

asio saturates at one io_context: its p50 degrades to 18–38 µs at 8×8/16×16
while the multi-poller xsl server keeps 7–11 µs and scales with the poller
count.

Per-request cost is CONSTANT over connection lifetime (30 s runs hold
~200 K RPS at 1×1); CPU/request ≈ 1.0 µs (multi-poller) / 1.5 µs (thread
pool), RSS flat (≈ 7 MB), zero minor page faults per request (previously
≈ 70/req and +3.2 KB RSS/req).

### optimization history (1×1 / 4×4 RPS, thread-pool model unless noted)

| stage                                   | 1×1            | 4×4          | note                     |
|-----------------------------------------|----------------|--------------|--------------------------|
| original (2025-12 measurements)         | ~20,500 / 44µs | ~105,000     | RPS decayed 24K→9K/25s   |
| + Message::read O(1) buffer reuse       | ~114,600 / 8µs | ~441,000     | root cause fix           |
| + writev assembly + cached Date         | ~143,700 / 6µs | ~569,400     | 1 syscall, no per-req strings |
| + SmallHeaderMap (POD header storage)   | ~142,000       | ~566,000     | zero header allocs, perf-neutral |
| + multi-poller (`-n 8`)                 | ~200,000 / 4µs | ~622,000     | = asio at 1×1, above at 16×16 |

## findings

1. **The dominant per-request cost was `Message::read` buffer mismanagement**
   (fixed): every read allocated a fresh 4 KiB block and the block was moved
   into the per-connection `Message::buffer` afterwards, growing it by one
   block per request. Each request then paid O(n) over the whole block
   history (`reserve(n)` + `insert(begin, ...)`) plus one minor page fault
   for the fresh page — measured 43–70 µs CPU/request growing linearly with
   requests served (RPS decayed from 24 K to 9 K over 25 s). This masked
   everything else: dispatch model experiments (Noop vs NewThread vs thread
   pool) all showed the same ~20 K RPS because the cost was in the parse
   path, not the scheduler.
2. The rewritten `Message::read` (see `test/unit/http/message.cpp`) reuses
   blocks with a rotate-on-full scheme and keeps the parsed views valid by
   only compacting at read() entry (the previous request's views are dead
   by then). Leftover bytes of a pipelined next head are preserved — the
   old implementation silently overwrote them.
3. **Micro-benchmarks of the remaining hot path** (per call, GCC 16):
   `RequestLine::parse` (incl. two `std::regex_match`) ≈ 490 ns,
   `to_date_string` ≈ 113 ns, response head build + `to_string` ≈ 114 ns,
   5 header map inserts ≈ 108 ns. Together ≈ 0.7 µs of the then-1.7 µs
   per-request CPU — the earlier hypotheses (regex too slow, date
   formatting, header maps dominating) were all falsified by measurement;
   they are minor next steps, not the bottleneck.
4. **TCP_NODELAY is essential.** Without `TCP_NODELAY` on the accepted socket,
   every keep-alive response after the first one stalls on the client's
   delayed ACK (~40 ms per request, ~25 RPS per connection — a 1000×
   regression). The xsl server sets `TCP_NODELAY` on accepted connections
   by default (`SocketOptions::no_delay`, applied in
   `AsyncConnectionUtils::accept`).
5. **Response head + body go out in a single `writev` syscall** for string
   bodies (`ResponseBuilder::_raw_body`, head rendered into the `sendto`
   coroutine frame via `ResponsePart::render_head`, zero heap allocations);
   streaming bodies (sendfile etc.) keep the callback path, and devices
   without `writev` (TLS) fall back to two writes. Together with a per-second
   thread-local cached Date string (`cached_date_string`) this took CPU per
   request from 1.7 µs to 1.5 µs and p50 from 8 µs to 6 µs.
6. **Response headers live in a POD inline array** (`SmallHeaderMap`, 8
   slots, 40/64-byte key/value buffers, linear scan, first-wins emplace,
   overflow vector for long values): zero per-request header allocations.
   A clean interleaved A/B against the unordered_map build shows loopback
   throughput within noise (≈ 200 K RPS at 1×1, CPU/request 1.01 → 0.99 µs
   at 8×8) — the benefit is structural (no allocator traffic, no rehash
   spikes) rather than a headline RPS win; an earlier string-slot variant
   was ~4.5% slower (frame growth + 16 std::string moves per response
   move) and was discarded for the POD design.
7. **The 1×1 latency gap to asio is closed by the multi-poller model**
   (`PollerGroup`, `bench_http_xsl -n N`): N listen sockets bound with
   SO_REUSEPORT, each poller thread runs its own IOContext with a
   NoopExecutor-bound context so coroutines resume INLINE on the thread that
   woke them — the dispatch hop disappears (1×1 p50 6 µs → 4 µs, equal to
   asio; CPU/request 1.5 → 1.0 µs) and throughput scales with the poller
   count where the single-threaded asio reference saturates (1.21 M vs
   409 K RPS at 16×16). Tradeoffs (documented in poller_group.h): a blocking
   handler stalls its poller, and the service object is used concurrently
   (read-only lookups are safe; handlers must not mutate shared captures).
8. The standalone asio server keeps 4 µs p50 by writing each response with a
   single `async_write` and parsing requests in place over one reusable
   buffer.

## correctness notes

`it_http_compare` (ctest) asserts that both servers answer identically for
`GET /hello`, keep-alive pipelining, `404` + `Connection: close` teardown,
and concurrent load — including a 2-poller `PollerGroup` variant of the xsl
server. It exists so that performance claims are only made about servers that
behave the same. Request-reader invariants (chunked delivery, pipelined
leftover, multi-block heads, O(1) buffer reuse) are pinned by
`test/unit/http/message.cpp`; header storage semantics by
`test/unit/http/header_map.cpp`.
