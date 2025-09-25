#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;
using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;

SCENARIO("Best bid/ask tracking with tick_type_strong", "[best_price_regression]")
{
    GIVEN("An empty order book")
    {
        VectorOrderBook book{};

        THEN("Initial best bid/ask should return sentinel values")
        {
            // NO_BID_VALUE and NO_ASK_VALUE are limits::min/max
            REQUIRE(book.best_bid() == std::numeric_limits<int>::min());
            REQUIRE(book.best_ask() == std::numeric_limits<int>::max());
        }

        WHEN("Adding first bid")
        {
            book.insert_bid(110, jazzy::tests::order{.order_id = 1, .volume = 100});

            THEN("Best bid should be updated")
            {
                REQUIRE(book.best_bid() == 110);
                REQUIRE(book.best_ask() == std::numeric_limits<int>::max()); // Still no asks
            }
        }

        WHEN("Adding multiple bids")
        {
            book.insert_bid(105, jazzy::tests::order{.order_id = 1, .volume = 100});
            book.insert_bid(110, jazzy::tests::order{.order_id = 2, .volume = 100});
            book.insert_bid(108, jazzy::tests::order{.order_id = 3, .volume = 100});

            THEN("Best bid should be highest") { REQUIRE(book.best_bid() == 110); }
        }

        WHEN("Adding first ask")
        {
            book.insert_ask(115, jazzy::tests::order{.order_id = 1, .volume = 100});

            THEN("Best ask should be updated")
            {
                REQUIRE(book.best_ask() == 115);
                REQUIRE(book.best_bid() == std::numeric_limits<int>::min()); // Still no bids
            }
        }

        WHEN("Adding multiple asks")
        {
            book.insert_ask(120, jazzy::tests::order{.order_id = 1, .volume = 100});
            book.insert_ask(115, jazzy::tests::order{.order_id = 2, .volume = 100});
            book.insert_ask(118, jazzy::tests::order{.order_id = 3, .volume = 100});

            THEN("Best ask should be lowest") { REQUIRE(book.best_ask() == 115); }
        }
    }
}

SCENARIO("Best price updates correctly when removing orders", "[best_price_regression]")
{
    GIVEN("Order book with multiple bid levels")
    {
        VectorOrderBook book{};

        // Add bids at different levels
        book.insert_bid(105, jazzy::tests::order{.order_id = 1, .volume = 100});
        book.insert_bid(110, jazzy::tests::order{.order_id = 2, .volume = 150}); // Best bid
        book.insert_bid(108, jazzy::tests::order{.order_id = 3, .volume = 200});

        REQUIRE(book.best_bid() == 110);

        WHEN("Removing the best bid completely")
        {
            book.remove_bid(110, jazzy::tests::order{.order_id = 2, .volume = 150});

            THEN("Best bid should be recalculated to next highest")
            {
                REQUIRE(book.best_bid() == 108);
                REQUIRE(book.bid_volume_at_tick(110) == 0);
                REQUIRE(book.bid_volume_at_tick(108) == 200);
            }
        }
    }

    GIVEN("Order book with multiple ask levels")
    {
        VectorOrderBook book{};

        // Add asks at different levels
        book.insert_ask(120, jazzy::tests::order{.order_id = 1, .volume = 100});
        book.insert_ask(115, jazzy::tests::order{.order_id = 2, .volume = 150}); // Best ask
        book.insert_ask(118, jazzy::tests::order{.order_id = 3, .volume = 200});

        REQUIRE(book.best_ask() == 115);

        WHEN("Removing the best ask completely")
        {
            book.remove_ask(115, jazzy::tests::order{.order_id = 2, .volume = 150});

            THEN("Best ask should be recalculated to next lowest")
            {
                REQUIRE(book.best_ask() == 118);
                REQUIRE(book.ask_volume_at_tick(115) == 0);
                REQUIRE(book.ask_volume_at_tick(118) == 200);
            }
        }
    }
}

SCENARIO("Best price recalculation when removing all orders", "[best_price_regression]")
{
    GIVEN("Order book with single bid and ask")
    {
        VectorOrderBook book{};

        book.insert_bid(110, jazzy::tests::order{.order_id = 1, .volume = 100});
        book.insert_ask(115, jazzy::tests::order{.order_id = 2, .volume = 100});

        REQUIRE(book.best_bid() == 110);
        REQUIRE(book.best_ask() == 115);

        WHEN("Removing the only bid")
        {
            book.remove_bid(110, jazzy::tests::order{.order_id = 1, .volume = 100});

            THEN("Best bid should return to sentinel value")
            {
                REQUIRE(book.best_bid() == std::numeric_limits<int>::min());
                REQUIRE(book.best_ask() == 115); // Ask unchanged
            }
        }

        WHEN("Removing the only ask")
        {
            book.remove_ask(115, jazzy::tests::order{.order_id = 2, .volume = 100});

            THEN("Best ask should return to sentinel value")
            {
                REQUIRE(book.best_ask() == std::numeric_limits<int>::max());
                REQUIRE(book.best_bid() == 110); // Bid unchanged
            }
        }
    }
}

SCENARIO("Best price tracking through update operations", "[best_price_regression]")
{
    GIVEN("Order book with established best prices")
    {
        VectorOrderBook book{};

        book.insert_bid(110, jazzy::tests::order{.order_id = 1, .volume = 100});
        book.insert_bid(108, jazzy::tests::order{.order_id = 2, .volume = 200});

        REQUIRE(book.best_bid() == 110);

        WHEN("Updating order to create new best bid")
        {
            book.update_bid(112, jazzy::tests::order{.order_id = 1, .volume = 150}); // Move order to better price

            THEN("Best bid should be updated")
            {
                REQUIRE(book.best_bid() == 112);
                REQUIRE(book.bid_volume_at_tick(112) == 150);
                REQUIRE(book.bid_volume_at_tick(110) == 0); // Old price should be empty
            }
        }

        WHEN("Updating order volume at same price")
        {
            book.update_bid(110, jazzy::tests::order{.order_id = 1, .volume = 250}); // Same price, different volume

            THEN("Best bid price should remain the same")
            {
                REQUIRE(book.best_bid() == 110);
                REQUIRE(book.bid_volume_at_tick(110) == 250);
            }
        }
    }
}

SCENARIO("tick_type_strong range validation in operations", "[best_price_regression]")
{
    GIVEN("Order book with market range 90-130")
    {
        VectorOrderBook book{};

        WHEN("Attempting operations outside valid range")
        {
            // These should be ignored (out of range)
            book.insert_bid(85, jazzy::tests::order{.order_id = 1, .volume = 100}); // Below 90
            book.insert_bid(135, jazzy::tests::order{.order_id = 2, .volume = 100}); // Above 130
            book.insert_ask(85, jazzy::tests::order{.order_id = 3, .volume = 100}); // Below 90
            book.insert_ask(135, jazzy::tests::order{.order_id = 4, .volume = 100}); // Above 130

            THEN("Orders should be rejected and best prices remain at sentinel values")
            {
                REQUIRE(book.best_bid() == std::numeric_limits<int>::min());
                REQUIRE(book.best_ask() == std::numeric_limits<int>::max());
                REQUIRE(book.bid_volume_at_tick(85) == 0);
                REQUIRE(book.bid_volume_at_tick(135) == 0);
                REQUIRE(book.ask_volume_at_tick(85) == 0);
                REQUIRE(book.ask_volume_at_tick(135) == 0);
            }
        }

        WHEN("Operating at valid range boundaries")
        {
            book.insert_bid(90, jazzy::tests::order{.order_id = 1, .volume = 100}); // Min valid
            book.insert_bid(130, jazzy::tests::order{.order_id = 2, .volume = 100}); // Max valid
            book.insert_ask(90, jazzy::tests::order{.order_id = 3, .volume = 100}); // Min valid
            book.insert_ask(130, jazzy::tests::order{.order_id = 4, .volume = 100}); // Max valid

            THEN("Orders should be accepted")
            {
                REQUIRE(book.best_bid() == 130); // Highest bid
                REQUIRE(book.best_ask() == 90); // Lowest ask
                REQUIRE(book.bid_volume_at_tick(90) == 100);
                REQUIRE(book.bid_volume_at_tick(130) == 100);
                REQUIRE(book.ask_volume_at_tick(90) == 100);
                REQUIRE(book.ask_volume_at_tick(130) == 100);
            }
        }
    }
}