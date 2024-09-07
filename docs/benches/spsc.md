# spsc benchmarks {#bench_spsc}

## comprarison target
- [x] **moodycamel::readerwriterqueue** [blog](https://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++) and [github](https://github.com/cameron314/readerwriterqueue)
- [x] **boost::lockfree::queue** [boost](https://www.boost.org/)

## command

### prepare

If you want to run accurate benchmarks, you should see some [tips](https://github.com/google/benchmark/blob/main/docs/user_guide.md#benchmarking-tips) from google benchmark.

If you use `xmake`, you can use the following command to prepare the benchmark data.
```bash
xmake f -m release -c --log_level=none
xmake build -vDg benchmarks/spsc
xmake run -vD bench_spsc --benchmark_format=json > test/benches/spsc/bench_result_xsl.json
xmake run -vD bench_boost_spsc --benchmark_format=json > test/benches/spsc/bench_result_boost.json
xmake run -vD bench_moodycamel_spsc --benchmark_format=json > test/benches/spsc/bench_result_moodycamel.json
```

### compare


```bash
test/benches/compare.py benchmarks test/benches/spsc/bench_result_boost.json test/benches/spsc/bench_result_xsl.json
test/benches/compare.py benchmarks test/benches/spsc/bench_result_moodycamel.json test/benches/spsc/bench_result_xsl.json
```

## benchmark result

compare with `boost::lockfree::queue`

![](../images/Screenshot_20240824_181222.png)
![](Screenshot_20240824_181222.png)

compare with `moodycamel::readerwriterqueue`

![](../images/Screenshot_20240824_182155.png)
![](Screenshot_20240824_182155.png)
