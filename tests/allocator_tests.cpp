#include <catch.hpp>
#include <cstddef>
#include <jazzy/order_book.hpp>
#include <memory>
#include <memory_resource>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

namespace
{
struct tracking_resource : std::pmr::memory_resource
{
    int* allocation_count;
    int* deallocation_count;
    int id;
    std::pmr::memory_resource* upstream;

    tracking_resource(
        int* alloc_count,
        int* dealloc_count,
        int alloc_id = 0,
        std::pmr::memory_resource* upstream_resource = std::pmr::get_default_resource())
        : allocation_count(alloc_count)
        , deallocation_count(dealloc_count)
        , id(alloc_id)
        , upstream(upstream_resource)
    {}

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override
    {
        if (allocation_count)
            ++(*allocation_count);
        return upstream->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
    {
        if (deallocation_count)
            ++(*deallocation_count);
        upstream->deallocate(p, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        if (this == &other)
            return true;
        if (auto* other_tracking = dynamic_cast<const tracking_resource*>(&other))
            return id == other_tracking->id;
        return false;
    }
};
} // namespace

SCENARIO("order books work with polymorphic allocators", "[orderbook][allocator]")
{
    using OrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;

    GIVEN("An order book with a tracking memory resource")
    {
        int alloc_count = 0;
        int dealloc_count = 0;
        tracking_resource resource{&alloc_count, &dealloc_count, 1};

        WHEN("Creating and destroying an order book")
        {
            {
                OrderBook book{&resource};
                book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
                book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});

                REQUIRE(alloc_count > 0);
            }

            THEN("All allocations are deallocated") { REQUIRE(alloc_count == dealloc_count); }
        }

        WHEN("Copy constructing an order book")
        {
            alloc_count = 0;
            dealloc_count = 0;

            OrderBook original{&resource};
            original.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

            int original_alloc_count = alloc_count;

            OrderBook copy{original};

            THEN("The copy uses select_on_container_copy_construction")
            {
                REQUIRE(alloc_count >= original_alloc_count);
                REQUIRE(copy.bid_volume_at_tick(100) == 10);
                REQUIRE(copy.best_bid() == 100);
                auto expected_allocator = std::allocator_traits<OrderBook::allocator_type>::select_on_container_copy_construction(
                    original.get_allocator());
                REQUIRE(copy.get_allocator() == expected_allocator);
            }
        }

        WHEN("Move constructing an order book")
        {
            alloc_count = 0;
            dealloc_count = 0;

            OrderBook original{&resource};
            original.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

            int alloc_before_move = alloc_count;

            OrderBook moved{std::move(original)};

            THEN("No new allocations occur (just transfer)")
            {
#ifdef _MSC_VER
                REQUIRE(alloc_count <= alloc_before_move + 2);
#else
                REQUIRE(alloc_count == alloc_before_move);
#endif
                REQUIRE(moved.bid_volume_at_tick(100) == 10);
                REQUIRE(moved.best_bid() == 100);
            }
        }
    }

    GIVEN("Order books with distinct polymorphic allocators")
    {
        int alloc_count1 = 0, dealloc_count1 = 0;
        int alloc_count2 = 0, dealloc_count2 = 0;

        tracking_resource resource1{&alloc_count1, &dealloc_count1, 1};
        tracking_resource resource2{&alloc_count2, &dealloc_count2, 2};

        OrderBook book1{&resource1};
        OrderBook book2{&resource2};

        book1.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book2.insert_bid(105, jazzy::tests::order{.order_id = 2, .volume = 20});

        WHEN("Copy assigning between books with different resources")
        {
            auto book2_allocator_before = book2.get_allocator();
            book2 = book1;

            THEN("Allocator remains unchanged while data is copied")
            {
                REQUIRE(book2.get_allocator() == book2_allocator_before);
                REQUIRE(book2.bid_volume_at_tick(100) == 10);
            }
        }

        WHEN("Move assigning between books with different resources")
        {
            auto book2_allocator_before = book2.get_allocator();
            book2 = std::move(book1);

            THEN("Allocator remains unchanged while values move")
            {
                REQUIRE(book2.get_allocator() == book2_allocator_before);
                REQUIRE(book2.bid_volume_at_tick(100) == 10);
            }
        }
    }
}

SCENARIO("order book allocator awareness is complete", "[orderbook][allocator]")
{
    using OrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;

    GIVEN("An order book with stateful memory resource")
    {
        int alloc_count = 0;
        int dealloc_count = 0;

        tracking_resource resource{&alloc_count, &dealloc_count, 42};

        OrderBook book{&resource};

        WHEN("Retrieving the allocator")
        {
            auto alloc = book.get_allocator();

            THEN("The correct allocator is returned")
            {
                REQUIRE(alloc.resource() == &resource);
            }
        }

        WHEN("Performing operations")
        {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.update_bid(101, jazzy::tests::order{.order_id = 1, .volume = 15});
            book.remove_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("All operations complete successfully with custom allocator")
            {
                REQUIRE(book.bid_volume_at_tick(101) == 15);
                REQUIRE(book.ask_volume_at_tick(110) == 0);
                REQUIRE(alloc_count > 0);
            }
        }
    }
}
