# net-cpp {#readme}

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)

A network library for C++.

## Features

- [x] Asynchronous I/O (epoll + C++20 coroutines, multi-poller server model)
- [x] TCP
- [x] HTTP/1.1 server & client (routing, static files, keep-alive)
- [x] spsc
- [ ] UDP

## Documentation

```bash
$ doxygen Doxyfile
```

## Benchmark

Please see the [benchmarks](./docs/benchmarks.md) and the
[HTTP server benchmark](./docs/benches/http.md) (xsl vs standalone asio:
parity at 1 connection, 3x throughput at 16).

## Usage

Please see the [examples](./docs/example.md).

## Architecture

See [docs/architecture.md](./docs/architecture.md) for the coroutine runtime,
IO/threading model and ownership invariants.
