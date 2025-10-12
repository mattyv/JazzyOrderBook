#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;
using fifo_storage = jazzy::detail::fifo_level_storage<jazzy::tests::order>;
using FifoOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats, fifo_storage>;
using FifoOrderBookDeletePolicy = jazzy::order_book<int, jazzy::tests::order, test_market_stats, fifo_storage, jazzy::detail::zero_volume_as_delete_policy>;
using FifoOrderBookValidPolicy = jazzy::order_book<int, jazzy::tests::order, test_market_stats, fifo_storage, jazzy::detail::zero_volume_as_valid_policy>;

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

SCENARIO("FIFO order book with delete policy removes zero-volume orders", "[orderbook][fifo][policy]")
{
    GIVEN("A FIFO order book with delete policy and a single bid order")
    {
        FifoOrderBookDeletePolicy book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

        WHEN("The order volume is updated to zero and a new order arrives at the same price")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 0});
            book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 5});

            THEN("The queue front should reflect the new live order")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 5);
            }

            THEN("The price level volume should only reflect the remaining order")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 5);
            }
        }
    }
}

SCENARIO("FIFO order book with valid policy keeps zero-volume orders", "[orderbook][fifo][policy]")
{
    GIVEN("A FIFO order book with valid policy and a single bid order")
    {
        FifoOrderBookValidPolicy book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

        WHEN("The order volume is updated to zero")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 0});

            THEN("The order should remain in the book with zero volume")
            {
                auto order = book.get_order(1);
                REQUIRE(order.order_id == 1);
                REQUIRE(order.volume == 0);
            }

            THEN("The price level volume should be zero")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 0);
            }

            THEN("Best bid should be empty since all orders have zero volume")
            {
                REQUIRE(book.best_bid() == std::numeric_limits<int>::min());
            }
        }

        WHEN("Volume is updated to zero then back to non-zero at the same price")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 0});
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 15});

            THEN("The order should still be in the queue with updated volume")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 15);
            }

            THEN("The price level volume should reflect the update")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 15);
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

SCENARIO("FIFO order book copy preserves queue iterators", "[orderbook][fifo][copy]")
{
    GIVEN("A FIFO order book with queued bids")
    {
        FifoOrderBook original{};
        original.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        original.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        original.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("The book is copy constructed")
        {
            FifoOrderBook copy{original};

            THEN("Removing the front order keeps queue integrity")
            {
                copy.remove_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

                auto front = copy.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(copy.bid_volume_at_tick(100) == 50);

                auto original_front = original.front_order_at_bid_level(0);
                REQUIRE(original_front.order_id == 1);
            }
        }

        WHEN("The book is copy assigned")
        {
            FifoOrderBook assigned{};
            assigned = original;

            THEN("Queue iterators are rebuilt in the assigned book")
            {
                assigned.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 15});

                auto front = assigned.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(assigned.bid_volume_at_tick(100) == 65);

                auto original_front = original.front_order_at_bid_level(0);
                REQUIRE(original_front.order_id == 1);
            }
        }
    }
}
