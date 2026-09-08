# Benchmarks {#benchmarks}

I use [google benchmark](https://github.com/google/benchmark) to benchmark the performance of the library. Just copy and change the `compare.py` script and `gbench` directory to compare the benchmark results.



## [http server: xsl::asio vs standalone asio](benches/http.md)

Coroutine HTTP server vs a hand-tuned asio reference, same load generator:
parity at 1 connection (4 µs p50), ~3x throughput at 16 connections
(SO_REUSEPORT multi-poller).

## [spsc](benches/spsc.md)
