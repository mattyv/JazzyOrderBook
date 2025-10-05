#include "../tests/order.hpp"
#include "map_order_book.hpp"
#include <benchmark/benchmark.h>
#include <jazzy/order_book.hpp>

#include <algorithm>
#include <random>
#include <vector>

// Define market stats for benchmarking
using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

// Random number generator for consistent benchmarking
thread_local std::mt19937 gen(42); // Fixed seed for reproducible results

// Helper functions to generate random orders within market range
jazzy::tests::order generate_random_order(int order_id)
{
    std::uniform_int_distribution<int> tick_dist(90, 130); // Market range
    std::uniform_int_distribution<int> volume_dist(1, 1000);

    return {order_id, volume_dist(gen), tick_dist(gen)};
}

// Helper to add order to book (randomly bid or ask)
template <typename BookType>
void add_random_order(BookType& book, const jazzy::tests::order& order)
{
    std::uniform_int_distribution<int> side_dist(0, 1);
    if (side_dist(gen) == 0)
    {
        book.insert_bid(order.tick, order);
    }
    else
    {
        book.insert_ask(order.tick, order);
    }
}

// Benchmark: Adding orders to empty order book
static void BM_JazzyOrderBook_AddOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        VectorOrderBook book{};

        // Add orders equal to the benchmark parameter
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            add_random_order(book, order);
        }

        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_AddOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        MapOrderBook book{};

        // Add orders equal to the benchmark parameter
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            add_random_order(book, order);
        }

        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

// Benchmark: Volume lookups in populated order book
static void BM_JazzyOrderBook_VolumeLookup(benchmark::State& state)
{
    VectorOrderBook book{};
    std::vector<int> ticks;

    // Pre-populate the order book
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i);
        ticks.push_back(order.tick);
        add_random_order(book, order);
    }

    // Shuffle ticks for random access pattern
    std::shuffle(ticks.begin(), ticks.end(), gen);

    for (auto _ : state)
    {
        // Look up volume at different price levels
        for (auto tick : ticks)
        {
            auto bid_volume = book.bid_volume_at_tick(tick);
            auto ask_volume = book.ask_volume_at_tick(tick);
            benchmark::DoNotOptimize(bid_volume);
            benchmark::DoNotOptimize(ask_volume);
        }
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_VolumeLookup(benchmark::State& state)
{
    MapOrderBook book{};
    std::vector<int> ticks;

    // Pre-populate the order book
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i);
        ticks.push_back(order.tick);
        add_random_order(book, order);
    }

    // Shuffle ticks for random access pattern
    std::shuffle(ticks.begin(), ticks.end(), gen);

    for (auto _ : state)
    {
        // Look up volume at different price levels
        for (auto tick : ticks)
        {
            auto bid_volume = book.bid_volume_at_tick(tick);
            auto ask_volume = book.ask_volume_at_tick(tick);
            benchmark::DoNotOptimize(bid_volume);
            benchmark::DoNotOptimize(ask_volume);
        }
    }
    state.SetComplexityN(state.range(0));
}

// Benchmark: Mixed operations (add/update/remove)
static void BM_JazzyOrderBook_MixedOps(benchmark::State& state)
{
    for (auto _ : state)
    {
        VectorOrderBook book{};
        std::vector<jazzy::tests::order> active_orders;

        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2); // 0=add, 1=update, 2=remove

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(gen);

            if (op == 0 || active_orders.empty())
            {
                // Add operation
                auto order = generate_random_order(static_cast<int>(i));
                active_orders.push_back(order);
                add_random_order(book, order);
            }
            else if (op == 1 && !active_orders.empty())
            {
                // Update operation - change volume
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto& order = active_orders[idx];

                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(gen);

                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                {
                    book.update_bid(order.tick, order);
                }
                else
                {
                    book.update_ask(order.tick, order);
                }
            }
            else if (op == 2 && !active_orders.empty())
            {
                // Remove operation
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto order = active_orders[idx];

                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                {
                    book.remove_bid(order.tick, order);
                }
                else
                {
                    book.remove_ask(order.tick, order);
                }

                active_orders.erase(active_orders.begin() + static_cast<std::ptrdiff_t>(idx));
            }
        }

        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_MixedOps(benchmark::State& state)
{
    for (auto _ : state)
    {
        MapOrderBook book{};
        std::vector<jazzy::tests::order> active_orders;

        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2); // 0=add, 1=update, 2=remove

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(gen);

            if (op == 0 || active_orders.empty())
            {
                // Add operation
                auto order = generate_random_order(static_cast<int>(i));
                active_orders.push_back(order);
                add_random_order(book, order);
            }
            else if (op == 1 && !active_orders.empty())
            {
                // Update operation - change volume
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto& order = active_orders[idx];

                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(gen);

                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                {
                    book.update_bid(order.tick, order);
                }
                else
                {
                    book.update_ask(order.tick, order);
                }
            }
            else if (op == 2 && !active_orders.empty())
            {
                // Remove operation
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto order = active_orders[idx];

                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                {
                    book.remove_bid(order.tick, order);
                }
                else
                {
                    book.remove_ask(order.tick, order);
                }

                active_orders.erase(active_orders.begin() + static_cast<std::ptrdiff_t>(idx));
            }
        }

        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

// Benchmark: Update orders with realistic distribution (75% in top 5 price levels)
static void BM_JazzyOrderBook_UpdateOrders(benchmark::State& state)
{
    VectorOrderBook book{};
    std::vector<jazzy::tests::order> hot_orders; // Top 5 price levels (near best bid/ask)
    std::vector<jazzy::tests::order> cold_orders; // Remaining price levels

    const int64_t total_orders = state.range(0);
    // Distribute orders: ~20% in top 5 levels, rest spread out
    const int64_t hot_count = std::max(int64_t(5), total_orders / 5);

    std::uniform_int_distribution<int> volume_dist(1, 1000);

    // Pre-populate hot orders: top 5 price levels (ticks 108-112 around mid 110)
    std::uniform_int_distribution<int> hot_tick_dist(108, 112);
    for (int i = 0; i < hot_count; ++i)
    {
        jazzy::tests::order order{i, volume_dist(gen), hot_tick_dist(gen)};
        hot_orders.push_back(order);
        add_random_order(book, order);
    }

    // Pre-populate cold orders: spread across full range
    std::uniform_int_distribution<int> cold_tick_dist(90, 130);
    for (int64_t i = hot_count; i < total_orders; ++i)
    {
        jazzy::tests::order order{static_cast<int>(i), volume_dist(gen), cold_tick_dist(gen)};
        cold_orders.push_back(order);
        add_random_order(book, order);
    }

    std::uniform_int_distribution<int> update_dist(0, 99); // 0-99 for percentage
    std::uniform_int_distribution<int> side_dist(0, 1);

    for (auto _ : state)
    {
        // Realistic: 75% of updates hit hot orders (top 5% of levels)
        for (int64_t i = 0; i < total_orders; ++i)
        {
            bool update_hot = update_dist(gen) < 75; // 75% chance
            auto& order = update_hot && !hot_orders.empty()
                ? hot_orders[static_cast<size_t>(i) % hot_orders.size()]
                : cold_orders[static_cast<size_t>(i) % std::max(size_t(1), cold_orders.size())];

            order.volume = volume_dist(gen);

            if (side_dist(gen) == 0)
            {
                book.update_bid(order.tick, order);
            }
            else
            {
                book.update_ask(order.tick, order);
            }
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_UpdateOrders(benchmark::State& state)
{
    MapOrderBook book{};
    std::vector<jazzy::tests::order> hot_orders; // Top 5 price levels (near best bid/ask)
    std::vector<jazzy::tests::order> cold_orders; // Remaining price levels

    const int64_t total_orders = state.range(0);
    // Distribute orders: ~20% in top 5 levels, rest spread out
    const int64_t hot_count = std::max(int64_t(5), total_orders / 5);

    std::uniform_int_distribution<int> volume_dist(1, 1000);

    // Pre-populate hot orders: top 5 price levels (ticks 108-112 around mid 110)
    std::uniform_int_distribution<int> hot_tick_dist(108, 112);
    for (int i = 0; i < hot_count; ++i)
    {
        jazzy::tests::order order{i, volume_dist(gen), hot_tick_dist(gen)};
        hot_orders.push_back(order);
        add_random_order(book, order);
    }

    // Pre-populate cold orders: spread across full range
    std::uniform_int_distribution<int> cold_tick_dist(90, 130);
    for (int64_t i = hot_count; i < total_orders; ++i)
    {
        jazzy::tests::order order{static_cast<int>(i), volume_dist(gen), cold_tick_dist(gen)};
        cold_orders.push_back(order);
        add_random_order(book, order);
    }

    std::uniform_int_distribution<int> update_dist(0, 99); // 0-99 for percentage
    std::uniform_int_distribution<int> side_dist(0, 1);

    for (auto _ : state)
    {
        // Realistic: 75% of updates hit hot orders (top 5% of levels)
        for (int64_t i = 0; i < total_orders; ++i)
        {
            bool update_hot = update_dist(gen) < 75; // 75% chance
            auto& order = update_hot && !hot_orders.empty()
                ? hot_orders[static_cast<size_t>(i) % hot_orders.size()]
                : cold_orders[static_cast<size_t>(i) % std::max(size_t(1), cold_orders.size())];

            order.volume = volume_dist(gen);

            if (side_dist(gen) == 0)
            {
                book.update_bid(order.tick, order);
            }
            else
            {
                book.update_ask(order.tick, order);
            }
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

// Benchmark: Delete orders from populated order book
static void BM_JazzyOrderBook_DeleteOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        VectorOrderBook book{};
        std::vector<jazzy::tests::order> orders;

        // Pre-populate the order book
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            orders.push_back(order);
            add_random_order(book, order);
        }

        // Time the deletion of all orders
        for (const auto& order : orders)
        {
            std::uniform_int_distribution<int> side_dist(0, 1);
            if (side_dist(gen) == 0)
            {
                book.remove_bid(order.tick, order);
            }
            else
            {
                book.remove_ask(order.tick, order);
            }
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_DeleteOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        MapOrderBook book{};
        std::vector<jazzy::tests::order> orders;

        // Pre-populate the order book
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            orders.push_back(order);
            add_random_order(book, order);
        }

        // Time the deletion of all orders
        for (const auto& order : orders)
        {
            std::uniform_int_distribution<int> side_dist(0, 1);
            if (side_dist(gen) == 0)
            {
                book.remove_bid(order.tick, order);
            }
            else
            {
                book.remove_ask(order.tick, order);
            }
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

// Benchmark: Get order at level operations
static void BM_JazzyOrderBook_GetOrderAtLevel(benchmark::State& state)
{
    VectorOrderBook book{};

    // Pre-populate the order book with orders at different levels
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i);
        add_random_order(book, order);
    }

    for (auto _ : state)
    {
        // Access orders at different levels
        const int max_levels = std::min(20, 50); // Conservative estimate of actual levels
        for (size_t level = 0; level < max_levels; ++level)
        {
            auto bid_order = book.bid_at_level(level);
            auto ask_order = book.ask_at_level(level);
            benchmark::DoNotOptimize(bid_order);
            benchmark::DoNotOptimize(ask_order);
        }
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_GetOrderAtLevel(benchmark::State& state)
{
    MapOrderBook book{};

    // Pre-populate the order book with orders at different levels
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i);
        add_random_order(book, order);
    }

    for (auto _ : state)
    {
        // Access orders at different levels
        const int max_levels = std::min(20, 50); // Conservative estimate of actual levels
        for (size_t level = 0; level < max_levels; ++level)
        {
            auto bid_order = book.bid_at_level(level);
            auto ask_order = book.ask_at_level(level);
            benchmark::DoNotOptimize(bid_order);
            benchmark::DoNotOptimize(ask_order);
        }
    }
    state.SetComplexityN(state.range(0));
}

// Register benchmarks with various sizes
BENCHMARK(BM_JazzyOrderBook_AddOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_AddOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_UpdateOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_UpdateOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_DeleteOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_DeleteOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_VolumeLookup)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_VolumeLookup)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_GetOrderAtLevel)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_GetOrderAtLevel)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_MixedOps)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_MixedOps)->Range(8, 8 << 10)->Complexity();

// FIFO vs Aggregate Storage Benchmarks
using fifo_storage = jazzy::detail::fifo_level_storage<jazzy::tests::order>;
using FifoOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats, fifo_storage>;

static void BM_AggregateOrderBook_AddOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        VectorOrderBook book{};
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            add_random_order(book, order);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_FifoOrderBook_AddOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        FifoOrderBook book{};
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            add_random_order(book, order);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_AggregateOrderBook_UpdateOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        VectorOrderBook book{};
        std::vector<jazzy::tests::order> orders;
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            orders.push_back(order);
            add_random_order(book, order);
        }

        std::uniform_int_distribution<int> volume_dist(1, 1000);
        for (auto& order : orders)
        {
            order.volume = volume_dist(gen);
            std::uniform_int_distribution<int> side_dist(0, 1);
            if (side_dist(gen) == 0)
                book.update_bid(order.tick, order);
            else
                book.update_ask(order.tick, order);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_FifoOrderBook_UpdateOrders(benchmark::State& state)
{
    for (auto _ : state)
    {
        FifoOrderBook book{};
        std::vector<jazzy::tests::order> orders;
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i);
            orders.push_back(order);
            add_random_order(book, order);
        }

        std::uniform_int_distribution<int> volume_dist(1, 1000);
        for (auto& order : orders)
        {
            order.volume = volume_dist(gen);
            std::uniform_int_distribution<int> side_dist(0, 1);
            if (side_dist(gen) == 0)
                book.update_bid(order.tick, order);
            else
                book.update_ask(order.tick, order);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_AggregateOrderBook_MixedOps(benchmark::State& state)
{
    for (auto _ : state)
    {
        VectorOrderBook book{};
        std::vector<jazzy::tests::order> active_orders;
        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2);

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(gen);
            if (op == 0 || active_orders.empty())
            {
                auto order = generate_random_order(static_cast<int>(i));
                active_orders.push_back(order);
                add_random_order(book, order);
            }
            else if (op == 1 && !active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto& order = active_orders[idx];
                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(gen);
                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                    book.update_bid(order.tick, order);
                else
                    book.update_ask(order.tick, order);
            }
            else if (!active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto order = active_orders[idx];
                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                    book.remove_bid(order.tick, order);
                else
                    book.remove_ask(order.tick, order);
                active_orders.erase(active_orders.begin() + static_cast<std::ptrdiff_t>(idx));
            }
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_FifoOrderBook_MixedOps(benchmark::State& state)
{
    for (auto _ : state)
    {
        FifoOrderBook book{};
        std::vector<jazzy::tests::order> active_orders;
        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2);

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(gen);
            if (op == 0 || active_orders.empty())
            {
                auto order = generate_random_order(static_cast<int>(i));
                active_orders.push_back(order);
                add_random_order(book, order);
            }
            else if (op == 1 && !active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto& order = active_orders[idx];
                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(gen);
                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                    book.update_bid(order.tick, order);
                else
                    book.update_ask(order.tick, order);
            }
            else if (!active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(gen);
                auto order = active_orders[idx];
                std::uniform_int_distribution<int> side_dist(0, 1);
                if (side_dist(gen) == 0)
                    book.remove_bid(order.tick, order);
                else
                    book.remove_ask(order.tick, order);
                active_orders.erase(active_orders.begin() + static_cast<std::ptrdiff_t>(idx));
            }
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_FifoOrderBook_FrontOrderLookup(benchmark::State& state)
{
    FifoOrderBook book{};
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i);
        add_random_order(book, order);
    }

    for (auto _ : state)
    {
        const int max_levels = std::min(20, 50);
        for (size_t level = 0; level < max_levels; ++level)
        {
            auto bid_order = book.front_order_at_bid_level(level);
            auto ask_order = book.front_order_at_ask_level(level);
            benchmark::DoNotOptimize(bid_order);
            benchmark::DoNotOptimize(ask_order);
        }
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_AggregateOrderBook_AddOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_FifoOrderBook_AddOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_AggregateOrderBook_UpdateOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_FifoOrderBook_UpdateOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_AggregateOrderBook_MixedOps)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_FifoOrderBook_MixedOps)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_FifoOrderBook_FrontOrderLookup)->Range(8, 8 << 10)->Complexity();

BENCHMARK_MAIN();