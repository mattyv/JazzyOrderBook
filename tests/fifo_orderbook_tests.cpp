#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;
using fifo_storage = jazzy::detail::fifo_level_storage<jazzy::tests::order, std::allocator<jazzy::tests::order>>;
using FifoOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats, std::allocator<jazzy::tests::order>, fifo_storage>;

SCENARIO("FIFO order book maintains order queue at each price level", "[orderbook][fifo]")
{
    GIVEN("An empty FIFO order book")
    {
        FifoOrderBook book{};

        WHEN("Multiple orders are inserted at the same bid price")
        {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The front order should be the first one inserted")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 10);
            }

            THEN("Total volume at the level should be correct")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 60);
            }
        }

        WHEN("Multiple orders are inserted at the same ask price")
        {
            book.insert_ask(110, jazzy::tests::order{.order_id = 4, .volume = 5});
            book.insert_ask(110, jazzy::tests::order{.order_id = 5, .volume = 15});
            book.insert_ask(110, jazzy::tests::order{.order_id = 6, .volume = 25});

            THEN("The front order should be the first one inserted")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 4);
                REQUIRE(front.volume == 5);
            }

            THEN("Total volume at the level should be correct")
            {
                REQUIRE(book.ask_volume_at_tick(110) == 45);
            }
        }
    }
}

SCENARIO("FIFO order book handles volume updates correctly", "[orderbook][fifo]")
{
    GIVEN("A FIFO order book with multiple orders at a price level")
    {
        FifoOrderBook book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("The middle order's volume is decreased")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 2, .volume = 15});

            THEN("It should maintain its position in the queue")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
            }

            THEN("Total volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 55);
            }
        }

        WHEN("The first order's volume is increased")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 25});

            THEN("It should move to the back of the queue")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }

            THEN("Total volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 75);
            }
        }
    }
}

SCENARIO("FIFO order book handles order removal correctly", "[orderbook][fifo]")
{
    GIVEN("A FIFO order book with multiple orders")
    {
        FifoOrderBook book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("The front order is removed")
        {
            book.remove_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The next order becomes the front")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }

            THEN("Total volume is updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 50);
            }
        }

        WHEN("A middle order is removed")
        {
            book.remove_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The front order remains unchanged")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
            }

            THEN("Total volume is updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 40);
            }
        }
    }
}

SCENARIO("FIFO order book handles price level changes", "[orderbook][fifo]")
{
    GIVEN("A FIFO order book with orders at different prices")
    {
        FifoOrderBook book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_bid(101, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("An order is moved to a different price level")
        {
            book.update_bid(101, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("It should be removed from the old queue")
            {
                auto front_100 = book.front_order_at_bid_level(1); // level 1 is price 100
                REQUIRE(front_100.order_id == 1);
            }

            THEN("It should be added to the back of the new queue")
            {
                auto front_101 = book.front_order_at_bid_level(0); // level 0 is best bid (101)
                REQUIRE(front_101.order_id == 3); // order 3 was inserted first at 101
            }

            THEN("Volumes should be correct at both levels")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 10);
                REQUIRE(book.bid_volume_at_tick(101) == 50);
            }
        }
    }
}

SCENARIO("FIFO order book handles ask side queue correctly", "[orderbook][fifo]")
{
    GIVEN("A FIFO order book with multiple ask orders")
    {
        FifoOrderBook book{};
        book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_ask(110, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("The first order's volume is increased")
        {
            book.update_ask(110, jazzy::tests::order{.order_id = 1, .volume = 25});

            THEN("It should move to the back of the queue")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 2);
            }
        }

        WHEN("The middle order is removed")
        {
            book.remove_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("Queue order is preserved")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(book.ask_volume_at_tick(110) == 40);
            }
        }
    }
}
