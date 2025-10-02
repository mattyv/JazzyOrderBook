#include <catch.hpp>
#include <cstddef>
#include <jazzy/order_book.hpp>
#include <memory>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

// Tracking allocator to verify proper propagation behavior
template <typename T>
struct tracking_allocator
{
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    int* allocation_count;
    int* deallocation_count;
    int id;

    tracking_allocator(int* alloc_count, int* dealloc_count, int alloc_id = 0)
        : allocation_count(alloc_count)
        , deallocation_count(dealloc_count)
        , id(alloc_id)
    {}

    template <typename U>
    tracking_allocator(const tracking_allocator<U>& other)
        : allocation_count(other.allocation_count)
        , deallocation_count(other.deallocation_count)
        , id(other.id)
    {}

    T* allocate(size_type n)
    {
        if (allocation_count)
            ++(*allocation_count);
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_type) noexcept
    {
        if (deallocation_count)
            ++(*deallocation_count);
        ::operator delete(p);
    }

    template <typename U>
    bool operator==(const tracking_allocator<U>& other) const noexcept
    {
        return id == other.id;
    }

    template <typename U>
    bool operator!=(const tracking_allocator<U>& other) const noexcept
    {
        return !(*this == other);
    }

    template <typename U>
    struct rebind
    {
        using other = tracking_allocator<U>;
    };
};

// Non-propagating allocator for testing
template <typename T>
struct non_propagating_allocator
{
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;

    int id;

    explicit non_propagating_allocator(int alloc_id = 0)
        : id(alloc_id)
    {}

    template <typename U>
    non_propagating_allocator(const non_propagating_allocator<U>& other)
        : id(other.id)
    {}

    T* allocate(size_type n) { return static_cast<T*>(::operator new(n * sizeof(T))); }

    void deallocate(T* p, size_type) noexcept { ::operator delete(p); }

    template <typename U>
    bool operator==(const non_propagating_allocator<U>& other) const noexcept
    {
        return id == other.id;
    }

    template <typename U>
    bool operator!=(const non_propagating_allocator<U>& other) const noexcept
    {
        return !(*this == other);
    }

    template <typename U>
    struct rebind
    {
        using other = non_propagating_allocator<U>;
    };
};

SCENARIO("order books work with stateful allocators", "[orderbook][allocator]")
{
    GIVEN("An order book with a tracking allocator")
    {
        int alloc_count = 0;
        int dealloc_count = 0;

        using OrderBook =
            jazzy::order_book<int, jazzy::tests::order, test_market_stats, tracking_allocator<jazzy::tests::order>>;

        WHEN("Creating and destroying an order book")
        {
            {
                OrderBook book{tracking_allocator<jazzy::tests::order>(&alloc_count, &dealloc_count, 1)};
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

            OrderBook original{tracking_allocator<jazzy::tests::order>(&alloc_count, &dealloc_count, 1)};
            original.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

            int original_alloc_count = alloc_count;

            OrderBook copy{original};

            THEN("The copy uses select_on_container_copy_construction")
            {
                // Should have allocated for the copy
                REQUIRE(alloc_count > original_alloc_count);

                // Both should have the same data
                REQUIRE(copy.bid_volume_at_tick(100) == 10);
                REQUIRE(copy.best_bid() == 100);
            }
        }

        WHEN("Move constructing an order book")
        {
            alloc_count = 0;
            dealloc_count = 0;

            OrderBook original{tracking_allocator<jazzy::tests::order>(&alloc_count, &dealloc_count, 1)};
            original.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

            int alloc_before_move = alloc_count;

            OrderBook moved{std::move(original)};

            THEN("No new allocations occur (just transfer)")
            {
// Move should not allocate new memory
// Note: MSVC's STL may allocate bookkeeping structures during move
#ifdef _MSC_VER
                REQUIRE(alloc_count <= alloc_before_move + 4); // Allow MSVC tolerance for hash map
#else
                REQUIRE(alloc_count == alloc_before_move);
#endif

                // Moved-to object has the data
                REQUIRE(moved.bid_volume_at_tick(100) == 10);
                REQUIRE(moved.best_bid() == 100);
            }
        }
    }

    GIVEN("Order books with propagating allocators")
    {
        int alloc_count1 = 0, dealloc_count1 = 0;
        int alloc_count2 = 0, dealloc_count2 = 0;

        using OrderBook =
            jazzy::order_book<int, jazzy::tests::order, test_market_stats, tracking_allocator<jazzy::tests::order>>;

        OrderBook book1{tracking_allocator<jazzy::tests::order>(&alloc_count1, &dealloc_count1, 1)};
        OrderBook book2{tracking_allocator<jazzy::tests::order>(&alloc_count2, &dealloc_count2, 2)};

        book1.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book2.insert_bid(105, jazzy::tests::order{.order_id = 2, .volume = 20});

        WHEN("Copy assigning with propagate_on_container_copy_assignment")
        {
            book2 = book1;

            THEN("Allocator is propagated")
            {
                // book2 should now use book1's allocator
                REQUIRE(book2.get_allocator() == book1.get_allocator());
                REQUIRE(book2.bid_volume_at_tick(100) == 10);
            }
        }

        WHEN("Move assigning with propagate_on_container_move_assignment")
        {
            auto original_allocator = book1.get_allocator();
            book2 = std::move(book1);

            THEN("Allocator is propagated")
            {
                REQUIRE(book2.get_allocator() == original_allocator);
                REQUIRE(book2.bid_volume_at_tick(100) == 10);
            }
        }
    }

    GIVEN("Order books with non-propagating allocators")
    {
        using OrderBook = jazzy::
            order_book<int, jazzy::tests::order, test_market_stats, non_propagating_allocator<jazzy::tests::order>>;

        OrderBook book1{non_propagating_allocator<jazzy::tests::order>(1)};
        OrderBook book2{non_propagating_allocator<jazzy::tests::order>(2)};

        book1.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book2.insert_bid(105, jazzy::tests::order{.order_id = 2, .volume = 20});

        WHEN("Copy assigning without propagation")
        {
            auto book2_allocator_before = book2.get_allocator();
            book2 = book1;

            THEN("Allocator is NOT propagated")
            {
                // book2 should keep its original allocator
                REQUIRE(book2.get_allocator() == book2_allocator_before);
                // But data is copied
                REQUIRE(book2.bid_volume_at_tick(100) == 10);
            }
        }

        WHEN("Move assigning with equal allocators")
        {
            OrderBook book3{non_propagating_allocator<jazzy::tests::order>(1)};
            book3.insert_bid(110, jazzy::tests::order{.order_id = 3, .volume = 30});

            auto book1_allocator_before = book1.get_allocator();
            book1 = std::move(book3);

            THEN("Move is efficient because allocators are equal")
            {
                REQUIRE(book1.get_allocator() == book1_allocator_before);
                REQUIRE(book1.bid_volume_at_tick(110) == 30);
            }
        }

        WHEN("Move assigning with unequal allocators")
        {
            auto book2_allocator_before = book2.get_allocator();
            book2 = std::move(book1);

            THEN("Must copy because allocators differ and don't propagate")
            {
                // book2 keeps its allocator
                REQUIRE(book2.get_allocator() == book2_allocator_before);
                // But data is transferred
                REQUIRE(book2.bid_volume_at_tick(100) == 10);
            }
        }
    }
}

SCENARIO("order book allocator awareness is complete", "[orderbook][allocator]")
{
    GIVEN("An order book with stateful allocator")
    {
        int alloc_count = 0;
        int dealloc_count = 0;

        using OrderBook =
            jazzy::order_book<int, jazzy::tests::order, test_market_stats, tracking_allocator<jazzy::tests::order>>;

        OrderBook book{tracking_allocator<jazzy::tests::order>(&alloc_count, &dealloc_count, 42)};

        WHEN("Retrieving the allocator")
        {
            auto alloc = book.get_allocator();

            THEN("The correct allocator is returned")
            {
                REQUIRE(alloc.id == 42);
                REQUIRE(alloc.allocation_count == &alloc_count);
                REQUIRE(alloc.deallocation_count == &dealloc_count);
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
