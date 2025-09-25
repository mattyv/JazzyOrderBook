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

        const int num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2); // 0=add, 1=update, 2=remove

        for (int i = 0; i < num_ops; ++i)
        {
            int op = op_dist(gen);

            if (op == 0 || active_orders.empty())
            {
                // Add operation
                auto order = generate_random_order(i);
                active_orders.push_back(order);
                add_random_order(book, order);
            }
            else if (op == 1 && !active_orders.empty())
            {
                // Update operation - change volume
                std::uniform_int_distribution<int> idx_dist(0, active_orders.size() - 1);
                int idx = idx_dist(gen);
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
                std::uniform_int_distribution<int> idx_dist(0, active_orders.size() - 1);
                int idx = idx_dist(gen);
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

                active_orders.erase(active_orders.begin() + idx);
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

        const int num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2); // 0=add, 1=update, 2=remove

        for (int i = 0; i < num_ops; ++i)
        {
            int op = op_dist(gen);

            if (op == 0 || active_orders.empty())
            {
                // Add operation
                auto order = generate_random_order(i);
                active_orders.push_back(order);
                add_random_order(book, order);
            }
            else if (op == 1 && !active_orders.empty())
            {
                // Update operation - change volume
                std::uniform_int_distribution<int> idx_dist(0, active_orders.size() - 1);
                int idx = idx_dist(gen);
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
                std::uniform_int_distribution<int> idx_dist(0, active_orders.size() - 1);
                int idx = idx_dist(gen);
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

                active_orders.erase(active_orders.begin() + idx);
            }
        }

        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

// Benchmark: Update orders in populated order book
static void BM_JazzyOrderBook_UpdateOrders(benchmark::State& state)
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

    for (auto _ : state)
    {
        // Update each order with new volume
        for (auto& order : orders)
        {
            std::uniform_int_distribution<int> volume_dist(1, 1000);
            order.volume = volume_dist(gen);

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
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

static void BM_MapOrderBook_UpdateOrders(benchmark::State& state)
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

    for (auto _ : state)
    {
        // Update each order with new volume
        for (auto& order : orders)
        {
            std::uniform_int_distribution<int> volume_dist(1, 1000);
            order.volume = volume_dist(gen);

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
        for (int level = 0; level < max_levels; ++level)
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
        for (int level = 0; level < max_levels; ++level)
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

BENCHMARK_MAIN();