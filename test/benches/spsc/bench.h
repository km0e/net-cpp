/**
 * @file bench.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Benchmark fixture for spsc queue.
 * @version 0.11
 * @date 2024-08-23
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <benchmark/benchmark.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <ranges>
#include <span>
#include <thread>
#include <vector>

#define MAX_DATA_SIZE 5000 * 1000

template <typename Spsc>
class MyFixture : public benchmark::Fixture {
private:
  std::vector<std::vector<int>> _data;
  std::size_t _index = 0;
  std::random_device rd;

public:
  MyFixture() : _data(), _index(0), rd() {}
  void SetUp(::benchmark::State& state) {
    if (state.range(0) > MAX_DATA_SIZE) {
      state.SkipWithError("Data size is too large");
    }
    _index = 0;
  }

  void TearDown(::benchmark::State&) {}

  std::span<int> random_data(std::size_t size, int start, int end) {
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(start, end);
    if (_index + 1 > _data.size()) {
      _data.emplace_back(MAX_DATA_SIZE);
    }
    auto span = std::span<int>(_data[_index].data(), size);
    _index++;
    for (auto& v : span) {
      v = dis(gen);
    }
    return span;
  }
  /**
   * @brief Mostly add operation
   *
   * @param state
   */
  void mostly_add(benchmark::State& state) {
    std::int64_t size = state.range(0);
    auto seq = random_data(size, 0, 3);
    auto data = random_data(size, 0, 1000);
    auto p_spsc
        = std::make_unique<Spsc>(size);  // eliminate interference caused by the memory address
    int count = 0;
    for (auto _ : state) {
      std::jthread producer([&] {
        for (auto i : data) {
          p_spsc->enqueue(i);
        }
      });
      std::jthread consumer([&] {
        for (auto r : seq) {
          if (r == 0) {
            int t = 0;
            p_spsc->try_dequeue(t);
            benchmark::DoNotOptimize(t);
            count++;
          }
        }
      });
      benchmark::DoNotOptimize(p_spsc);
      benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * data.size() + count);
  } /**
     * @brief Mostly remove operation
     *
     * @param state
     */
  void mostly_remove(benchmark::State& state) {
    std::int64_t size = state.range(0);
    auto seq = random_data(size, 0, 3);
    auto data = random_data(size, 0, 1000);
    auto p_spsc
        = std::make_unique<Spsc>(size);  // eliminate interference caused by the memory address
    int count = 0;
    for (auto _ : state) {
      std::jthread producer([&] {
        for (auto [r, v] : std::views::zip(seq, data)) {
          if (r == 0) {
            p_spsc->enqueue(v);
            count++;
          }
        }
      });
      std::jthread consumer([&] {
        for (auto _ : data) {
          int t = 0;
          p_spsc->try_dequeue(t);
          benchmark::DoNotOptimize(t);
        }
      });
      benchmark::DoNotOptimize(p_spsc);
      benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * data.size() + count);
  }
  /**
   * @brief Heavy concurrent operation
   *
   * @param state
   */
  void heavy_concurrent(benchmark::State& state) {
    std::int64_t size = state.range(0);
    auto data = random_data(size, 0, 1000);
    auto p_spsc
        = std::make_unique<Spsc>(size);  // eliminate interference caused by the memory address
    for (auto _ : state) {
      std::jthread producer([&] {
        for (auto i : data) {
          p_spsc->enqueue(i);
        }
      });
      std::jthread consumer([&] {
        for (auto _ : data) {
          int t = 0;
          p_spsc->try_dequeue(t);
          benchmark::DoNotOptimize(t);
        }
      });
      benchmark::DoNotOptimize(p_spsc);
      benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * data.size() * 2);
  } /**
     * @brief Random concurrent operation
     *
     * @param state
     */
  void random_concurrent(benchmark::State& state) {
    std::int64_t size = state.range(0);
    auto seq1 = random_data(size, 0, 15);
    auto seq2 = random_data(size, 0, 15);
    auto p_spsc
        = std::make_unique<Spsc>(size);  // eliminate interference caused by the memory address
    std::size_t read_count = 0, write_count = 0;
    for (auto _ : state) {
      std::jthread producer1([&] {
        for (auto i : seq1) {
          if (i == 0) {
            p_spsc->enqueue(i);
            write_count++;
          }
        }
      });
      std::jthread consumer1([&] {
        for (auto i : seq2) {
          if (i == 0) {
            int t = 0;
            p_spsc->try_dequeue(t);
            benchmark::DoNotOptimize(t);
            read_count++;
          }
        }
      });
      benchmark::DoNotOptimize(p_spsc);
      benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(read_count + write_count);
  }
};
/**
 * @brief Adapter for spsc queue
 *
 * @tparam Spsc
 */
template <typename Spsc>
struct spsc_adapter {
  spsc_adapter(std::size_t size) : _spsc(size) {}
  decltype(auto) enqueue(int i) { return _spsc.push(i); }
  decltype(auto) try_dequeue(int& i) { return _spsc.pop(i); }
  Spsc _spsc;
};
