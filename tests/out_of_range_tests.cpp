#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

SCENARIO("order book handles out of range bid operations", "[orderbook][out_of_range]")
{
    GIVEN("An empty order book with daily_high=130 and daily_low=90")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        WHEN("A bid is inserted above daily_high")
        {
            book.insert_bid(131, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The bid should be ignored and volume at that tick should be 0")
            {
                REQUIRE(book.bid_volume_at_tick(131) == 0);
            }
        }

        WHEN("A bid is inserted below daily_low")
        {
            book.insert_bid(89, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The bid should be ignored and volume at that tick should be 0")
            {
                REQUIRE(book.bid_volume_at_tick(89) == 0);
            }
        }

        WHEN("A bid is inserted at daily_high boundary")
        {
            book.insert_bid(130, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The bid should be accepted") { REQUIRE(book.bid_volume_at_tick(130) == 30); }
        }

        WHEN("A bid is inserted at daily_low boundary")
        {
            book.insert_bid(90, jazzy::tests::order{.order_id = 4, .volume = 40});

            THEN("The bid should be accepted") { REQUIRE(book.bid_volume_at_tick(90) == 40); }
        }
    }
}

SCENARIO("order book handles out of range ask operations", "[orderbook][out_of_range]")
{
    GIVEN("An empty order book with daily_high=130 and daily_low=90")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        WHEN("An ask is inserted above daily_high")
        {
            book.insert_ask(131, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The ask should be ignored and volume at that tick should be 0")
            {
                REQUIRE(book.ask_volume_at_tick(131) == 0);
            }
        }

        WHEN("An ask is inserted below daily_low")
        {
            book.insert_ask(89, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The ask should be ignored and volume at that tick should be 0")
            {
                REQUIRE(book.ask_volume_at_tick(89) == 0);
            }
        }

        WHEN("An ask is inserted at daily_high boundary")
        {
            book.insert_ask(130, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The ask should be accepted") { REQUIRE(book.ask_volume_at_tick(130) == 30); }
        }

        WHEN("An ask is inserted at daily_low boundary")
        {
            book.insert_ask(90, jazzy::tests::order{.order_id = 4, .volume = 40});

            THEN("The ask should be accepted") { REQUIRE(book.ask_volume_at_tick(90) == 40); }
        }
    }
}

SCENARIO("order book handles out of range update operations", "[orderbook][out_of_range]")
{
    GIVEN("An order book with existing valid orders")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        // Add valid orders first
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(120, jazzy::tests::order{.order_id = 2, .volume = 20});

        WHEN("Trying to update a bid to an out of range price above daily_high")
        {
            book.update_bid(131, jazzy::tests::order{.order_id = 1, .volume = 15});

            THEN("The update should be ignored and original volume remains")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 10);
                REQUIRE(book.bid_volume_at_tick(131) == 0);
            }
        }

        WHEN("Trying to update a bid to an out of range price below daily_low")
        {
            book.update_bid(89, jazzy::tests::order{.order_id = 1, .volume = 15});

            THEN("The update should be ignored and original volume remains")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 10);
                REQUIRE(book.bid_volume_at_tick(89) == 0);
            }
        }

        WHEN("Trying to update an ask to an out of range price above daily_high")
        {
            book.update_ask(131, jazzy::tests::order{.order_id = 2, .volume = 25});

            THEN("The update should be ignored and original volume remains")
            {
                REQUIRE(book.ask_volume_at_tick(120) == 20);
                REQUIRE(book.ask_volume_at_tick(131) == 0);
            }
        }

        WHEN("Trying to update an ask to an out of range price below daily_low")
        {
            book.update_ask(89, jazzy::tests::order{.order_id = 2, .volume = 25});

            THEN("The update should be ignored and original volume remains")
            {
                REQUIRE(book.ask_volume_at_tick(120) == 20);
                REQUIRE(book.ask_volume_at_tick(89) == 0);
            }
        }
    }
}

SCENARIO("order book handles out of range remove operations", "[orderbook][out_of_range]")
{
    GIVEN("An order book with existing valid orders")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        // Add valid orders first
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(120, jazzy::tests::order{.order_id = 2, .volume = 20});

        WHEN("Trying to remove a bid with an out of range price above daily_high")
        {
            book.remove_bid(131, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The remove should be ignored and original order remains")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 10);
                REQUIRE(book.bid_volume_at_tick(131) == 0);
            }
        }

        WHEN("Trying to remove a bid with an out of range price below daily_low")
        {
            book.remove_bid(89, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The remove should be ignored and original order remains")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 10);
                REQUIRE(book.bid_volume_at_tick(89) == 0);
            }
        }

        WHEN("Trying to remove an ask with an out of range price above daily_high")
        {
            book.remove_ask(131, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The remove should be ignored and original order remains")
            {
                REQUIRE(book.ask_volume_at_tick(120) == 20);
                REQUIRE(book.ask_volume_at_tick(131) == 0);
            }
        }

        WHEN("Trying to remove an ask with an out of range price below daily_low")
        {
            book.remove_ask(89, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The remove should be ignored and original order remains")
            {
                REQUIRE(book.ask_volume_at_tick(120) == 20);
                REQUIRE(book.ask_volume_at_tick(89) == 0);
            }
        }
    }
}

SCENARIO("order book volume queries for out of range ticks", "[orderbook][out_of_range]")
{
    GIVEN("An order book with some valid orders")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(120, jazzy::tests::order{.order_id = 2, .volume = 20});

        WHEN("Querying bid volume for out of range ticks")
        {
            THEN("Volume should be 0 for ticks above daily_high")
            {
                REQUIRE(book.bid_volume_at_tick(131) == 0);
                REQUIRE(book.bid_volume_at_tick(135) == 0);
                REQUIRE(book.bid_volume_at_tick(1000) == 0);
            }

            THEN("Volume should be 0 for ticks below daily_low")
            {
                REQUIRE(book.bid_volume_at_tick(89) == 0);
                REQUIRE(book.bid_volume_at_tick(85) == 0);
                REQUIRE(book.bid_volume_at_tick(1) == 0);
            }
        }

        WHEN("Querying ask volume for out of range ticks")
        {
            THEN("Volume should be 0 for ticks above daily_high")
            {
                REQUIRE(book.ask_volume_at_tick(131) == 0);
                REQUIRE(book.ask_volume_at_tick(135) == 0);
                REQUIRE(book.ask_volume_at_tick(1000) == 0);
            }

            THEN("Volume should be 0 for ticks below daily_low")
            {
                REQUIRE(book.ask_volume_at_tick(89) == 0);
                REQUIRE(book.ask_volume_at_tick(85) == 0);
                REQUIRE(book.ask_volume_at_tick(1) == 0);
            }
        }
    }
}

SCENARIO("order book handles mixed valid and out of range operations", "[orderbook][out_of_range]")
{
    GIVEN("An empty order book")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        WHEN("Multiple orders are added, some valid and some out of range")
        {
            // Valid orders
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(120, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_bid(95, jazzy::tests::order{.order_id = 3, .volume = 30});
            book.insert_ask(125, jazzy::tests::order{.order_id = 4, .volume = 40});

            // Out of range orders (should be ignored)
            book.insert_bid(131, jazzy::tests::order{.order_id = 5, .volume = 50});
            book.insert_ask(89, jazzy::tests::order{.order_id = 6, .volume = 60});
            book.insert_bid(85, jazzy::tests::order{.order_id = 7, .volume = 70});
            book.insert_ask(135, jazzy::tests::order{.order_id = 8, .volume = 80});

            THEN("Only valid orders should be present in the book")
            {
                // Valid orders should be there
                REQUIRE(book.bid_volume_at_tick(100) == 10);
                REQUIRE(book.ask_volume_at_tick(120) == 20);
                REQUIRE(book.bid_volume_at_tick(95) == 30);
                REQUIRE(book.ask_volume_at_tick(125) == 40);

                // Out of range orders should be ignored
                REQUIRE(book.bid_volume_at_tick(131) == 0);
                REQUIRE(book.ask_volume_at_tick(89) == 0);
                REQUIRE(book.bid_volume_at_tick(85) == 0);
                REQUIRE(book.ask_volume_at_tick(135) == 0);
            }
        }
    }
}