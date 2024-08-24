/**
 * @file spsc.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-24
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "../bench.h"
#include "readerwriterqueue.h"

#include <benchmark/benchmark.h>

using spsc_t = moodycamel::ReaderWriterQueue<int>;

BENCHMARK_TEMPLATE_DEFINE_F(MyFixture, mostly_add, spsc_t)(benchmark::State& state) {
  mostly_add(state);
}
BENCHMARK_REGISTER_F(MyFixture, mostly_add)
    ->Arg(1000 * 10)
    ->Arg(1000 * 100)
    ->Arg(1000 * 1000)
    ->Arg(1000 * 4000);

BENCHMARK_TEMPLATE_DEFINE_F(MyFixture, mostly_remove, spsc_t)(benchmark::State& state) {
  mostly_remove(state);
}
BENCHMARK_REGISTER_F(MyFixture, mostly_remove)
    ->Arg(1000 * 10)
    ->Arg(1000 * 100)
    ->Arg(1000 * 1000)
    ->Arg(1000 * 4000);

BENCHMARK_TEMPLATE_DEFINE_F(MyFixture, heavy_concurrent, spsc_t)(benchmark::State& state) {
  heavy_concurrent(state);
}
BENCHMARK_REGISTER_F(MyFixture, heavy_concurrent)
    ->Arg(1000 * 10)
    ->Arg(1000 * 100)
    ->Arg(1000 * 1000)
    ->Arg(1000 * 4000);

BENCHMARK_TEMPLATE_DEFINE_F(MyFixture, random_concurrent, spsc_t)(benchmark::State& state) {
  random_concurrent(state);
}
BENCHMARK_REGISTER_F(MyFixture, random_concurrent)
    ->Arg(1000 * 10)
    ->Arg(1000 * 100)
    ->Arg(1000 * 1000)
    ->Arg(1000 * 4000);
