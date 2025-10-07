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

namespace {

constexpr unsigned int BENCHMARK_SEED = 42u;

[[nodiscard]] std::mt19937 make_rng()
{
    return std::mt19937{BENCHMARK_SEED};
}

template <typename Book>
void recycle_pool(std::vector<Book>& books, std::size_t& next_idx, benchmark::State& state)
{
    if (next_idx < books.size())
    {
        return;
    }

    state.PauseTiming();
    for (auto& book : books)
    {
        book.clear();
    }
    next_idx = 0;
    state.ResumeTiming();
}

} // namespace

// Random number generator for consistent benchmarking
jazzy::tests::order generate_random_order(int order_id, std::mt19937& rng)
{
    std::uniform_int_distribution<int> tick_dist(90, 130); // Market range
    std::uniform_int_distribution<int> volume_dist(1, 1000);

    return {order_id, volume_dist(rng), tick_dist(rng)};
}

// Helper to add order to book (randomly bid or ask)
template <typename BookType>
bool add_random_order(BookType& book, const jazzy::tests::order& order, std::mt19937& rng)
{
    std::uniform_int_distribution<int> side_dist(0, 1);
    if (side_dist(rng) == 0)
    {
        book.insert_bid(order.tick, order);
        return true;
    }
    else
    {
        book.insert_ask(order.tick, order);
        return false;
    }
}

// Fixture for benchmarks with pre-created orderbook pool (eliminates pause/resume overhead)
class OrderBookFixture : public benchmark::Fixture {
public:
    static constexpr std::size_t POOL_SIZE = 1000;
    std::vector<VectorOrderBook> jazzy_books;
    std::vector<MapOrderBook> map_books;
    std::size_t book_idx = 0;

    void SetUp(const benchmark::State&) override {
        book_idx = 0;
        if (jazzy_books.empty())
        {
            jazzy_books.reserve(POOL_SIZE);
            map_books.reserve(POOL_SIZE);
            for (std::size_t i = 0; i < POOL_SIZE; ++i)
            {
                jazzy_books.emplace_back();
                map_books.emplace_back();
            }
        }
        for (auto& book : jazzy_books)
        {
            book.clear();
        }
        for (auto& book : map_books)
        {
            book.clear();
        }
    }
};

// Forward declare FIFO types for fixture
using fifo_storage = jazzy::detail::fifo_level_storage<jazzy::tests::order>;
using FifoOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats, fifo_storage>;

// Fixture for FIFO benchmarks
class FifoOrderBookFixture : public benchmark::Fixture {
public:
    static constexpr std::size_t POOL_SIZE = 1000;
    std::vector<VectorOrderBook> aggregate_books;
    std::vector<FifoOrderBook> fifo_books;
    std::size_t book_idx = 0;

    void SetUp(const benchmark::State&) override {
        book_idx = 0;
        if (aggregate_books.empty())
        {
            aggregate_books.reserve(POOL_SIZE);
            fifo_books.reserve(POOL_SIZE);
            for (std::size_t i = 0; i < POOL_SIZE; ++i) {
                aggregate_books.emplace_back();
                fifo_books.emplace_back();
            }
        }
        for (auto& book : aggregate_books)
        {
            book.clear();
        }
        for (auto& book : fifo_books)
        {
            book.clear();
        }
    }
};

// Benchmark: Adding orders to empty order book
BENCHMARK_DEFINE_F(OrderBookFixture, JazzyAddOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(jazzy_books, book_idx, state);
        auto& book = jazzy_books[book_idx++];

        // Add orders equal to the benchmark parameter
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            (void)add_random_order(book, order, rng);
        }

        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(OrderBookFixture, MapAddOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(map_books, book_idx, state);
        auto& book = map_books[book_idx++];

        // Add orders equal to the benchmark parameter
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            (void)add_random_order(book, order, rng);
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
    auto rng = make_rng();

    // Pre-populate the order book
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i, rng);
        ticks.push_back(order.tick);
        (void)add_random_order(book, order, rng);
    }

    // Shuffle ticks for random access pattern
    std::shuffle(ticks.begin(), ticks.end(), rng);

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
    auto rng = make_rng();

    // Pre-populate the order book
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i, rng);
        ticks.push_back(order.tick);
        (void)add_random_order(book, order, rng);
    }

    // Shuffle ticks for random access pattern
    std::shuffle(ticks.begin(), ticks.end(), rng);

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
BENCHMARK_DEFINE_F(OrderBookFixture, JazzyMixedOps)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(jazzy_books, book_idx, state);
        auto& book = jazzy_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> active_orders; // pair: (order, is_bid)

        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2); // 0=add, 1=update, 2=remove

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(rng);

            if (op == 0 || active_orders.empty())
            {
                // Add operation
                auto order = generate_random_order(static_cast<int>(i), rng);
                bool is_bid = add_random_order(book, order, rng);
                active_orders.emplace_back(order, is_bid);
            }
            else if (op == 1 && !active_orders.empty())
            {
                // Update operation - change volume
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(rng);
                auto& [order, is_bid] = active_orders[idx];

                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(rng);

                if (is_bid)
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
                size_t idx = idx_dist(rng);
                auto [order, is_bid] = active_orders[idx];

                if (is_bid)
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

BENCHMARK_DEFINE_F(OrderBookFixture, MapMixedOps)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(map_books, book_idx, state);
        auto& book = map_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> active_orders; // pair: (order, is_bid)

        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2); // 0=add, 1=update, 2=remove

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(rng);

            if (op == 0 || active_orders.empty())
            {
                // Add operation
                auto order = generate_random_order(static_cast<int>(i), rng);
                bool is_bid = add_random_order(book, order, rng);
                active_orders.emplace_back(order, is_bid);
            }
            else if (op == 1 && !active_orders.empty())
            {
                // Update operation - change volume
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(rng);
                auto& [order, is_bid] = active_orders[idx];

                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(rng);

                if (is_bid)
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
                size_t idx = idx_dist(rng);
                auto [order, is_bid] = active_orders[idx];

                if (is_bid)
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
    auto setup_rng = make_rng();

    const int64_t total_orders = state.range(0);
    // Distribute orders: ~20% in top 5 levels, rest spread out
    const int64_t hot_count = std::max(int64_t(5), total_orders / 5);

    std::uniform_int_distribution<int> volume_dist(1, 1000);

    // Pre-populate hot orders: top 5 price levels (ticks 108-112 around mid 110)
    std::uniform_int_distribution<int> hot_tick_dist(108, 112);
    for (int i = 0; i < hot_count; ++i)
    {
        jazzy::tests::order order{i, volume_dist(setup_rng), hot_tick_dist(setup_rng)};
        hot_orders.push_back(order);
        (void)add_random_order(book, order, setup_rng);
    }

    // Pre-populate cold orders: spread across full range
    std::uniform_int_distribution<int> cold_tick_dist(90, 130);
    for (int64_t i = hot_count; i < total_orders; ++i)
    {
        jazzy::tests::order order{static_cast<int>(i), volume_dist(setup_rng), cold_tick_dist(setup_rng)};
        cold_orders.push_back(order);
        (void)add_random_order(book, order, setup_rng);
    }

    std::uniform_int_distribution<int> update_dist(0, 99); // 0-99 for percentage
    std::uniform_int_distribution<int> side_dist(0, 1);
    auto rng = make_rng();

    for (auto _ : state)
    {
        // Realistic: 75% of updates hit hot orders (top 5% of levels)
        for (int64_t i = 0; i < total_orders; ++i)
        {
            bool update_hot = update_dist(rng) < 75; // 75% chance
            auto& order = update_hot && !hot_orders.empty()
                ? hot_orders[static_cast<size_t>(i) % hot_orders.size()]
                : cold_orders[static_cast<size_t>(i) % std::max(size_t(1), cold_orders.size())];

            order.volume = volume_dist(rng);

            if (side_dist(rng) == 0)
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
    auto setup_rng = make_rng();

    const int64_t total_orders = state.range(0);
    // Distribute orders: ~20% in top 5 levels, rest spread out
    const int64_t hot_count = std::max(int64_t(5), total_orders / 5);

    std::uniform_int_distribution<int> volume_dist(1, 1000);

    // Pre-populate hot orders: top 5 price levels (ticks 108-112 around mid 110)
    std::uniform_int_distribution<int> hot_tick_dist(108, 112);
    for (int i = 0; i < hot_count; ++i)
    {
        jazzy::tests::order order{i, volume_dist(setup_rng), hot_tick_dist(setup_rng)};
        hot_orders.push_back(order);
        (void)add_random_order(book, order, setup_rng);
    }

    // Pre-populate cold orders: spread across full range
    std::uniform_int_distribution<int> cold_tick_dist(90, 130);
    for (int64_t i = hot_count; i < total_orders; ++i)
    {
        jazzy::tests::order order{static_cast<int>(i), volume_dist(setup_rng), cold_tick_dist(setup_rng)};
        cold_orders.push_back(order);
        (void)add_random_order(book, order, setup_rng);
    }

    std::uniform_int_distribution<int> update_dist(0, 99); // 0-99 for percentage
    std::uniform_int_distribution<int> side_dist(0, 1);
    auto rng = make_rng();

    for (auto _ : state)
    {
        // Realistic: 75% of updates hit hot orders (top 5% of levels)
        for (int64_t i = 0; i < total_orders; ++i)
        {
            bool update_hot = update_dist(rng) < 75; // 75% chance
            auto& order = update_hot && !hot_orders.empty()
                ? hot_orders[static_cast<size_t>(i) % hot_orders.size()]
                : cold_orders[static_cast<size_t>(i) % std::max(size_t(1), cold_orders.size())];

            order.volume = volume_dist(rng);

            if (side_dist(rng) == 0)
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
BENCHMARK_DEFINE_F(OrderBookFixture, JazzyDeleteOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(jazzy_books, book_idx, state);
        auto& book = jazzy_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> orders; // pair: (order, is_bid)

        // Pre-populate the order book (NOT TIMED)
        state.PauseTiming();
        book.clear();
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            bool is_bid = add_random_order(book, order, rng);
            orders.emplace_back(order, is_bid);
        }
        state.ResumeTiming();

        // Time the deletion of all orders (ONLY THIS IS TIMED)
        for (const auto& [order, is_bid] : orders)
        {
            if (is_bid)
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

BENCHMARK_DEFINE_F(OrderBookFixture, MapDeleteOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(map_books, book_idx, state);
        auto& book = map_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> orders; // pair: (order, is_bid)

        // Pre-populate the order book (NOT TIMED)
        state.PauseTiming();
        book.clear();
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            bool is_bid = add_random_order(book, order, rng);
            orders.emplace_back(order, is_bid);
        }
        state.ResumeTiming();

        // Time the deletion of all orders (ONLY THIS IS TIMED)
        for (const auto& [order, is_bid] : orders)
        {
            if (is_bid)
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
    auto rng = make_rng();

    // Pre-populate the order book with orders at different levels
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i, rng);
        (void)add_random_order(book, order, rng);
    }

    for (auto _ : state)
    {
        // Access orders at different levels - scale with input but cap at 20
        const size_t max_levels = std::min(size_t(20), static_cast<size_t>(state.range(0)));
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
    auto rng = make_rng();

    // Pre-populate the order book with orders at different levels
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i, rng);
        (void)add_random_order(book, order, rng);
    }

    for (auto _ : state)
    {
        // Access orders at different levels - scale with input but cap at 20
        const size_t max_levels = std::min(size_t(20), static_cast<size_t>(state.range(0)));
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
BENCHMARK_REGISTER_F(OrderBookFixture, JazzyAddOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK_REGISTER_F(OrderBookFixture, MapAddOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_UpdateOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_UpdateOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK_REGISTER_F(OrderBookFixture, JazzyDeleteOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK_REGISTER_F(OrderBookFixture, MapDeleteOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_VolumeLookup)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_VolumeLookup)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_JazzyOrderBook_GetOrderAtLevel)->Range(8, 8 << 10)->Complexity();
BENCHMARK(BM_MapOrderBook_GetOrderAtLevel)->Range(8, 8 << 10)->Complexity();

BENCHMARK_REGISTER_F(OrderBookFixture, JazzyMixedOps)->Range(8, 8 << 10)->Complexity();
BENCHMARK_REGISTER_F(OrderBookFixture, MapMixedOps)->Range(8, 8 << 10)->Complexity();

// FIFO vs Aggregate Storage Benchmarks
BENCHMARK_DEFINE_F(FifoOrderBookFixture, AggregateAddOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(aggregate_books, book_idx, state);
        auto& book = aggregate_books[book_idx++];

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            (void)add_random_order(book, order, rng);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(FifoOrderBookFixture, FifoAddOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(fifo_books, book_idx, state);
        auto& book = fifo_books[book_idx++];

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            (void)add_random_order(book, order, rng);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(FifoOrderBookFixture, AggregateUpdateOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(aggregate_books, book_idx, state);
        auto& book = aggregate_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> orders; // pair: (order, is_bid)

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            bool is_bid = add_random_order(book, order, rng);
            orders.emplace_back(order, is_bid);
        }

        std::uniform_int_distribution<int> volume_dist(1, 1000);
        for (auto& [order, is_bid] : orders)
        {
            order.volume = volume_dist(rng);
            if (is_bid)
                book.update_bid(order.tick, order);
            else
                book.update_ask(order.tick, order);
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(FifoOrderBookFixture, FifoUpdateOrders)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(fifo_books, book_idx, state);
        auto& book = fifo_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> orders; // pair: (order, is_bid)

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = generate_random_order(i, rng);
            bool is_bid = add_random_order(book, order, rng);
            orders.emplace_back(order, is_bid);
        }

        std::uniform_int_distribution<int> volume_dist(1, 1000);
        for (auto& [order, is_bid] : orders)
        {
            auto current = book.get_order(order.order_id);
            current.volume = volume_dist(rng);
            if (is_bid)
                book.update_bid(current.tick, current);
            else
                book.update_ask(current.tick, current);
            order = current;
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(FifoOrderBookFixture, AggregateMixedOps)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(aggregate_books, book_idx, state);
        auto& book = aggregate_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> active_orders; // pair: (order, is_bid)

        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2);

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(rng);
            if (op == 0 || active_orders.empty())
            {
                auto order = generate_random_order(static_cast<int>(i), rng);
                bool is_bid = add_random_order(book, order, rng);
                active_orders.emplace_back(order, is_bid);
            }
            else if (op == 1 && !active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(rng);
                auto& [order, is_bid] = active_orders[idx];
                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                order.volume = new_volume_dist(rng);
                if (is_bid)
                    book.update_bid(order.tick, order);
                else
                    book.update_ask(order.tick, order);
            }
            else if (!active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(rng);
                auto [order, is_bid] = active_orders[idx];
                if (is_bid)
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

BENCHMARK_DEFINE_F(FifoOrderBookFixture, FifoMixedOps)(benchmark::State& state)
{
    auto rng = make_rng();
    for (auto _ : state)
    {
        recycle_pool(fifo_books, book_idx, state);
        auto& book = fifo_books[book_idx++];
        std::vector<std::pair<jazzy::tests::order, bool>> active_orders; // pair: (order, is_bid)

        const std::int64_t num_ops = state.range(0);
        std::uniform_int_distribution<int> op_dist(0, 2);

        for (std::int64_t i = 0; i < num_ops; ++i)
        {
            int op = op_dist(rng);
            if (op == 0 || active_orders.empty())
            {
                auto order = generate_random_order(static_cast<int>(i), rng);
                bool is_bid = add_random_order(book, order, rng);
                active_orders.emplace_back(order, is_bid);
            }
            else if (op == 1 && !active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(rng);
                auto& [order, is_bid] = active_orders[idx];
                std::uniform_int_distribution<int> new_volume_dist(1, 500);
                auto current = book.get_order(order.order_id);
                current.volume = new_volume_dist(rng);
                if (is_bid)
                    book.update_bid(current.tick, current);
                else
                    book.update_ask(current.tick, current);
                order = current;
            }
            else if (!active_orders.empty())
            {
                std::uniform_int_distribution<size_t> idx_dist(0, active_orders.size() - 1);
                size_t idx = idx_dist(rng);
                auto [order, is_bid] = active_orders[idx];
                auto current = book.get_order(order.order_id);
                if (is_bid)
                    book.remove_bid(current.tick, current);
                else
                    book.remove_ask(current.tick, current);
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
    auto rng = make_rng();
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = generate_random_order(i, rng);
        (void)add_random_order(book, order, rng);
    }

    for (auto _ : state)
    {
        // Use actual number of levels in the book, capped at 20
        const size_t max_bid_levels = book.bid_bitmap().count();
        const size_t max_ask_levels = book.ask_bitmap().count();
        const size_t max_levels = std::min({max_bid_levels, max_ask_levels, size_t(20)});

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

BENCHMARK_REGISTER_F(FifoOrderBookFixture, AggregateAddOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK_REGISTER_F(FifoOrderBookFixture, FifoAddOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK_REGISTER_F(FifoOrderBookFixture, AggregateUpdateOrders)->Range(8, 8 << 10)->Complexity();
BENCHMARK_REGISTER_F(FifoOrderBookFixture, FifoUpdateOrders)->Range(8, 8 << 10)->Complexity();

BENCHMARK_REGISTER_F(FifoOrderBookFixture, AggregateMixedOps)->Range(8, 8 << 10)->Complexity();
BENCHMARK_REGISTER_F(FifoOrderBookFixture, FifoMixedOps)->Range(8, 8 << 10)->Complexity();

BENCHMARK(BM_FifoOrderBook_FrontOrderLookup)->Range(8, 8 << 10)->Complexity();

BENCHMARK_MAIN();
