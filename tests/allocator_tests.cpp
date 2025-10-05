#include <catch.hpp>
#include <cstddef>
#include <jazzy/order_book.hpp>
#include <memory_resource>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

namespace {

struct counting_resource : std::pmr::memory_resource
{
    std::pmr::memory_resource* upstream;
    std::size_t allocations{0};
    std::size_t deallocations{0};

    explicit counting_resource(std::pmr::memory_resource* parent = std::pmr::get_default_resource()) noexcept
        : upstream(parent)
    {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override
    {
        ++allocations;
        return upstream->allocate(bytes, alignment);
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
    {
        ++deallocations;
        upstream->deallocate(ptr, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

} // namespace

TEST_CASE("order book uses supplied memory resource", "[orderbook][pmr]")
{
    counting_resource resource;

    jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{&resource};

    REQUIRE(book.get_memory_resource() == &resource);

    book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
    book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});

    REQUIRE(resource.allocations > 0);
    REQUIRE(resource.allocations >= resource.deallocations);
}

TEST_CASE("copy construction keeps the same resource", "[orderbook][pmr]")
{
    counting_resource resource;

    jazzy::order_book<int, jazzy::tests::order, test_market_stats> original{&resource};
    original.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

    jazzy::order_book<int, jazzy::tests::order, test_market_stats> copy{original};

    REQUIRE(copy.get_memory_resource() == &resource);
    REQUIRE(copy.bid_volume_at_tick(100) == original.bid_volume_at_tick(100));
}

TEST_CASE("copy construction into explicit resource", "[orderbook][pmr]")
{
    counting_resource source_resource;
    counting_resource target_resource;

    jazzy::order_book<int, jazzy::tests::order, test_market_stats> source{&source_resource};
    source.insert_bid(101, jazzy::tests::order{.order_id = 10, .volume = 15});

    jazzy::order_book<int, jazzy::tests::order, test_market_stats> copy{source, &target_resource};

    REQUIRE(copy.get_memory_resource() == &target_resource);
    REQUIRE(copy.bid_volume_at_tick(101) == source.bid_volume_at_tick(101));
    REQUIRE(target_resource.allocations > 0);
}

TEST_CASE("copy assignment preserves destination resource", "[orderbook][pmr]")
{
    counting_resource lhs_resource;
    counting_resource rhs_resource;

    jazzy::order_book<int, jazzy::tests::order, test_market_stats> lhs{&lhs_resource};
    jazzy::order_book<int, jazzy::tests::order, test_market_stats> rhs{&rhs_resource};

    rhs.insert_bid(102, jazzy::tests::order{.order_id = 20, .volume = 25});
    rhs.insert_ask(112, jazzy::tests::order{.order_id = 30, .volume = 35});

    auto* const before = lhs.get_memory_resource();
    lhs = rhs;

    REQUIRE(lhs.get_memory_resource() == before);
    REQUIRE(lhs.bid_volume_at_tick(102) == rhs.bid_volume_at_tick(102));
    REQUIRE(lhs.ask_volume_at_tick(112) == rhs.ask_volume_at_tick(112));
}

TEST_CASE("default constructor uses default resource", "[orderbook][pmr]")
{
    jazzy::order_book<int, jazzy::tests::order, test_market_stats> book;
    REQUIRE(book.get_memory_resource() == std::pmr::get_default_resource());
}
