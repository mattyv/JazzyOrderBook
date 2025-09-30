# JazzyOrderBook

JazzyOrderBook is a modern C++23 library for building fast limit-order book
aggregations. It keeps price levels in a densely packed vector sized from
compile-time market statistics, which allows constant-time volume updates and
cheap scans to the best bid/ask.

## Highlights
- Header-only core exposed through the `jazzy::jazzy` CMake target (requires
  CMake 3.21+ and a C++23 compiler).
- Strongly typed ticks and compile-time `market_statistics` describing daily
  high/low/close data plus an expected trading range.
- `jazzy::order_book` stores bid/ask levels in contiguous arrays augmented with
  bitsets to track populated levels and accelerate best price discovery.
- Concept-based traits let you adapt your own order struct by providing
  `jazzy_order_*` free functions; misuse is caught at compile time.
- Reference `map_order_book` baseline together with a Google Benchmark harness
  for performance comparisons.
- Comprehensive Catch2 regression suite covering inserts, updates, deletes, and
  best-price queries.

## Usage

```cpp
#include <cstdint>
#include <jazzy/order_book.hpp>
#include <jazzy/types.hpp>

struct order
{
    std::int64_t order_id{};
    std::int64_t volume{};
    int tick{};
};

inline std::int64_t jazzy_order_id_getter(order const& o) { return o.order_id; }
inline std::int64_t jazzy_order_volume_getter(order const& o) { return o.volume; }
inline int jazzy_order_tick_getter(order const& o) { return o.tick; }
inline void jazzy_order_volume_setter(order& o, std::int64_t v) { o.volume = v; }
inline void jazzy_order_tick_setter(order& o, int v) { o.tick = v; }

using stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;
using book_t = jazzy::order_book<int, order, stats>;

book_t book{};
book.insert_bid(101, order{.order_id = 1, .volume = 10});
book.insert_ask(105, order{.order_id = 2, .volume = 12});

auto best_bid = book.bid_at_level(0);
auto best_ask = book.ask_at_level(0);
```

## Building

1. Configure the project:

   ```bash
   cmake -S . -B build
   ```

2. Build the headers, tests, and benchmarks:

   ```bash
   cmake --build build
   ```

You can embed JazzyOrderBook into another CMake project by adding this
repository as a subdirectory and linking against `jazzy::jazzy`.

## Tests

The test suite is driven by Catch2 (the single-header dependency is vendored in
`tests/`). To build and run the tests:

```bash
cmake --build build --target tests
./build/tests/tests
```

Alternatively, execute all tests with CTest:

```bash
ctest --test-dir build
```

The regression scenarios exercise order insertion, modification, removal,
best-price scanning, range validation, and the `tick_type_strong` utility.

## Benchmarks

Benchmarks compare `jazzy::order_book` with the tree-based reference
implementation located in `benchmarks/map_order_book.hpp`:

```bash
cmake --build build --target benchmarks
./build/benchmarks/benchmarks
```

Sample outputs are stored under `benchmark_results/`. The helper script
`scripts/benchmark_compare.sh` runs the benchmark, captures the result keyed by
your CPU (using `scripts/detect_hardware.sh`), and lets you diff against the
current best run.

## Project Layout

- `include/jazzy/` – public headers (`order_book.hpp`, `traits.hpp`,
  `types.hpp`).
- `tests/` – Catch2 regression and property tests together with the sample
  `tests/order.hpp` adapter.
- `benchmarks/` – Google Benchmark harness and the map-based baseline book.
- `include/jazzy/detail/select_nth.hpp` – branch-friendly selection helper used inside the
  order book.
- `include/jazzy/detail/level_bitmap.hpp` – compact bitmap wrapper enabling fast level scans.
- `scripts/` – helper scripts for benchmarks and local tooling.
- `benchmark_results/` – archived benchmark runs (keyed by detected hardware).

## License

Licensed under the Apache License, Version 2.0. See `LICENSE` for details.

## CI

[![CI](https://github.com/mattyv/JazzyOrderBook/workflows/CI/badge.svg)](https://github.com/mattyv/JazzyOrderBook/actions)
