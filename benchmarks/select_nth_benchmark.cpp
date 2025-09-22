#include "select_nth.hpp"
#include <benchmark/benchmark.h>

using namespace select64;

static void BM_SelectNthSetBit(benchmark::State& state)
{
    uint64_t mask = 0x5555555555555555ULL; // alternating bits

    for (auto _ : state)
    {
        int result = select_nth_set_bit(mask, 5);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_SelectNthSetBit);