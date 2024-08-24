#!/bin/bash
xmake f -m release -c --log_level=none
xmake build -vDg benchmarks/spsc
xmake run --workdir=build -vD bench_spsc --benchmark_out_format=json --benchmark_out=bench_result_spsc.json
xmake run --workdir=build -vD bench_boost_spsc --benchmark_out_format=json --benchmark_out=bench_result_boost_spsc.json
xmake run --workdir=build -vD bench_moodycamel_spsc --benchmark_out_format=json --benchmark_out=bench_result_moodycamel_spsc.json
test/benches/compare.py benchmarks build/bench_result_boost_spsc.json build/bench_result_spsc.json
test/benches/compare.py benchmarks build/bench_result_moodycamel_spsc.json build/bench_result_spsc.json
