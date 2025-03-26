#include "ds.hpp"
#include <benchmark/benchmark.h>

static void BM_do_something(benchmark::State & state)
{
  for(auto _ : state)
  {
    std::vector<int> foo;
    benchmark::DoNotOptimize(foo = {func2(0)});
    benchmark::ClobberMemory();
  }
}

BENCHMARK(BM_do_something);
BENCHMARK_MAIN();
