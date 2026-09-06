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

A correctness comparison (both servers must behave identically) runs as a
ctest integration test: `test/integration/http_compare/compare.cpp`
(`it_http_compare`), which covers 200/keep-alive/404+close and concurrent
requests against both servers.

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
`GET /hello`, zero errors in all runs.

| scenario                    | xsl::asio         | standalone asio    | ratio |
|-----------------------------|-------------------|--------------------|-------|
| 4 threads × 4 conns, RPS    | ~105,000          | ~400,000           | 3.8×  |
| 4 threads × 4 conns, p50    | 34 µs             | 9 µs               | 3.8×  |
| 4 threads × 4 conns, p99    | 96 µs             | 13 µs              | 7.4×  |
| 1 thread × 1 conn, RPS      | ~20,500           | ~200,000           | 9.8×  |
| 1 thread × 1 conn, p50      | 47 µs             | 4 µs               | 11.8× |

## findings

- **TCP_NODELAY is essential.** Without `TCP_NODELAY` on the accepted socket,
  every keep-alive response after the first one stalls on the client's
  delayed ACK (~40 ms per request, ~25 RPS per connection — a 1000×
  regression). The xsl server now sets `TCP_NODELAY` on accepted connections
  by default (`SocketOptions::no_delay`, applied in
  `AsyncConnectionUtils::accept`).
- The remaining ~4–10× gap is the cost of the coroutine runtime's current
  design: `std::function`-type-erased handlers and per-request allocations
  (response built into `std::string`, `us_map` header maps, date formatting,
  and the head/body pair being written as two separate `write()` syscalls).
- **Dispatch model is NOT the bottleneck.** Measured with the same load
  generator (1×1 and 4×4, 5 s each):

  | executor                      | 1×1 RPS | 4×4 RPS  |
  |-------------------------------|---------|----------|
  | `NewThreadExecutor` (default) | 20,544  | 104,819  |
  | `NoopExecutor` (inline)       | 19,228  | 82,913   |
  | standalone asio (1 thread)    | 200,299 | 399,238  |

  Inline resumption (`NoopExecutor`) is semantically valid and works, but is
  no faster than thread-per-wakeup, and loses ~20% at 4×4 because every
  resumed coroutine then shares the poller thread (no parallelism, 16 cores
  idle). The ~50 µs per-request fixed cost is dominated by per-request
  allocations and the double `write()`, not by dispatch. Priority: single
  `writev` response assembly and cheaper header/date handling first, then a
  pool/inline executor for multi-core scaling (`NoopExecutor` pins the whole
  server to one core).
- The standalone asio server keeps 4 µs p50 by writing each response with a
  single `async_write` and parsing requests in place over one reusable
  buffer, and also sets `TCP_NODELAY` on accepted sockets.

## correctness notes

`it_http_compare` (ctest) asserts that both servers answer identically for
`GET /hello`, keep-alive pipelining, `404` + `Connection: close` teardown,
and concurrent load. It exists so that performance claims are only made about
servers that behave the same.
