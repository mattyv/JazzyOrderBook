#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

SCENARIO("order books can have bid orders added", "[orderbook]")
{
    GIVEN("An empty order book")
    {
        static constexpr int base = 110;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));
        REQUIRE(book.size() == expected_size);

        WHEN("A buy side order is added")
        {
            book.insert_bid(101, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("Then the level should reflect the quantity") { REQUIRE(book.bid_volume_at_tick(101) == 10); }
        }

        WHEN("multiple buy side orders are added")
        {
            book.insert_bid(101, jazzy::tests::order{.order_id = 1, .volume = 1});
            book.insert_bid(102, jazzy::tests::order{.order_id = 2, .volume = 2});
            book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 3});

            book.insert_bid(99, jazzy::tests::order{.order_id = 4, .volume = 4});
            book.insert_bid(98, jazzy::tests::order{.order_id = 5, .volume = 5});
            book.insert_bid(97, jazzy::tests::order{.order_id = 6, .volume = 6});

            THEN("Then the level should reflect the quantity")
            {
                REQUIRE(book.bid_volume_at_tick(101) == 1);
                REQUIRE(book.bid_volume_at_tick(102) == 2);
                REQUIRE(book.bid_volume_at_tick(103) == 3);

                REQUIRE(book.bid_volume_at_tick(99) == 4);
                REQUIRE(book.bid_volume_at_tick(98) == 5);
                REQUIRE(book.bid_volume_at_tick(97) == 6);

                REQUIRE(book.bid_at_level(0).volume == 3); // highest bid (103)
                REQUIRE(book.bid_at_level(1).volume == 2); // next highest bid (102)
                REQUIRE(book.bid_at_level(2).volume == 1); // next highest bid (101)
                REQUIRE(book.bid_at_level(3).volume == 4); // next highest bid (99)
                REQUIRE(book.bid_at_level(4).volume == 5); // next highest bid (98)
                REQUIRE(book.bid_at_level(5).volume == 6); // lowest bid (97)
            }
        }
    }

    GIVEN("An order book with a number of bid orders")
    {
        static constexpr int base = 110;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));
        REQUIRE(book.size() == expected_size);

        book.insert_bid(101, jazzy::tests::order{.order_id = 1, .volume = 1});
        book.insert_bid(102, jazzy::tests::order{.order_id = 2, .volume = 2});
        book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 3});

        book.insert_bid(99, jazzy::tests::order{.order_id = 4, .volume = 4});
        book.insert_bid(98, jazzy::tests::order{.order_id = 5, .volume = 5});
        book.insert_bid(97, jazzy::tests::order{.order_id = 6, .volume = 6});
        book.insert_bid(97, jazzy::tests::order{.order_id = 7, .volume = 4});

        WHEN("A bids order quantity is increased")
        {
            book.update_bid(97, jazzy::tests::order{.order_id = 6, .volume = 10});
            THEN("Then the level should reflect the adjustment")
            {
                REQUIRE(book.bid_volume_at_tick(97) == 14);
                REQUIRE(book.bid_at_level(5).volume == 14); // lowest bid level
            }
        }

        WHEN("A bids order quantity is decreased")
        {
            book.update_bid(97, jazzy::tests::order{.order_id = 6, .volume = 4});
            THEN("Then the level should reflect the adjustment")
            {
                REQUIRE(book.bid_volume_at_tick(97) == 8);
                REQUIRE(book.bid_at_level(5).volume == 8); // lowest bid level
            }
        }
    }

    GIVEN("An order book with a number of ask orders")
    {
        static constexpr int base = 90;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));
        REQUIRE(book.size() == expected_size);

        book.insert_ask(101, jazzy::tests::order{.order_id = 1, .volume = 1});
        book.insert_ask(102, jazzy::tests::order{.order_id = 2, .volume = 2});
        book.insert_ask(103, jazzy::tests::order{.order_id = 3, .volume = 3});

        book.insert_ask(99, jazzy::tests::order{.order_id = 4, .volume = 4});
        book.insert_ask(98, jazzy::tests::order{.order_id = 5, .volume = 5});
        book.insert_ask(97, jazzy::tests::order{.order_id = 6, .volume = 6});
        book.insert_ask(97, jazzy::tests::order{.order_id = 7, .volume = 4});

        WHEN("A ask order quantity is increased")
        {
            book.update_ask(97, jazzy::tests::order{.order_id = 6, .volume = 10});
            THEN("Then the level should reflect the adjustment") { REQUIRE(book.ask_volume_at_tick(97) == 14); }
        }

        WHEN("A ask order quantity is decreased")
        {
            book.update_ask(97, jazzy::tests::order{.order_id = 6, .volume = 4});
            THEN("Then the level should reflect the adjustment") { REQUIRE(book.ask_volume_at_tick(97) == 8); }
        }
    }
}

SCENARIO("order books can have ask orders added", "[orderbook]")
{
    GIVEN("An empty order book")
    {
        static constexpr int base = 110;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));
        REQUIRE(book.size() == expected_size);

        WHEN("A sell side order is added")
        {
            book.insert_ask(115, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("Then the level should reflect the quantity") { REQUIRE(book.ask_volume_at_tick(115) == 10); }
        }

        WHEN("multiple sell side orders are added")
        {
            book.insert_ask(115, jazzy::tests::order{.order_id = 1, .volume = 1});
            book.insert_ask(116, jazzy::tests::order{.order_id = 2, .volume = 2});
            book.insert_ask(117, jazzy::tests::order{.order_id = 3, .volume = 3});

            book.insert_ask(120, jazzy::tests::order{.order_id = 4, .volume = 4});
            book.insert_ask(121, jazzy::tests::order{.order_id = 5, .volume = 5});
            book.insert_ask(122, jazzy::tests::order{.order_id = 6, .volume = 6});

            THEN("Then the level should reflect the quantity")
            {
                REQUIRE(book.ask_volume_at_tick(115) == 1);
                REQUIRE(book.ask_volume_at_tick(116) == 2);
                REQUIRE(book.ask_volume_at_tick(117) == 3);

                REQUIRE(book.ask_volume_at_tick(120) == 4);
                REQUIRE(book.ask_volume_at_tick(121) == 5);
                REQUIRE(book.ask_volume_at_tick(122) == 6);

                REQUIRE(book.ask_at_level(0).volume == 1); // lowest ask (115)
                REQUIRE(book.ask_at_level(1).volume == 2); // next lowest ask (116)
                REQUIRE(book.ask_at_level(2).volume == 3); // next lowest ask (117)
                REQUIRE(book.ask_at_level(3).volume == 4); // next lowest ask (120)
                REQUIRE(book.ask_at_level(4).volume == 5); // next lowest ask (121)
                REQUIRE(book.ask_at_level(5).volume == 6); // highest ask (122)
            }
        }
    }
    GIVEN("A non empty order book")
    {
        static constexpr int base = 110;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));
        REQUIRE(book.size() == expected_size);

        book.insert_ask(115, jazzy::tests::order{.order_id = 1, .volume = 1});
        book.insert_ask(116, jazzy::tests::order{.order_id = 2, .volume = 2});
        book.insert_ask(117, jazzy::tests::order{.order_id = 3, .volume = 3});

        book.insert_ask(120, jazzy::tests::order{.order_id = 4, .volume = 4});
        book.insert_ask(121, jazzy::tests::order{.order_id = 5, .volume = 5});
        book.insert_ask(122, jazzy::tests::order{.order_id = 6, .volume = 6});
        book.insert_ask(122, jazzy::tests::order{.order_id = 7, .volume = 4});

        WHEN("An ask orders quantity is increased")
        {
            book.update_ask(122, jazzy::tests::order{.order_id = 6, .volume = 10});
            THEN("Then the level should reflect the adjustment")
            {
                REQUIRE(book.ask_volume_at_tick(122) == 14);
                REQUIRE(book.ask_at_level(5).volume == 14); // highest ask level
            }
        }
    }
}

SCENARIO("order books can have bid orders modified", "[orderbook]")
{
    GIVEN("An order book with multiple bid orders")
    {
        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        book.insert_bid(105, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(104, jazzy::tests::order{.order_id = 2, .volume = 15});
        book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 20});

        REQUIRE(book.bid_volume_at_tick(105) == 10);
        REQUIRE(book.bid_volume_at_tick(104) == 15);
        REQUIRE(book.bid_volume_at_tick(103) == 20);

        WHEN("A bid order quantity is increased")
        {
            book.update_bid(105, jazzy::tests::order{.order_id = 1, .volume = 25});

            THEN("The level should reflect the increase")
            {
                REQUIRE(book.bid_volume_at_tick(105) == 25);
                REQUIRE(book.bid_volume_at_tick(104) == 15);
                REQUIRE(book.bid_volume_at_tick(103) == 20);

                REQUIRE(book.bid_at_level(0).volume == 25); // highest bid (105)
                REQUIRE(book.bid_at_level(1).volume == 15); // next highest bid (104)
                REQUIRE(book.bid_at_level(2).volume == 20); // lowest bid (103)
            }
        }

        WHEN("A bid order quantity is decreased")
        {
            book.update_bid(104, jazzy::tests::order{.order_id = 2, .volume = 5});

            THEN("The level should reflect the decrease")
            {
                REQUIRE(book.bid_volume_at_tick(105) == 10);
                REQUIRE(book.bid_volume_at_tick(104) == 5);
                REQUIRE(book.bid_volume_at_tick(103) == 20);

                REQUIRE(book.bid_at_level(0).volume == 10); // highest bid (105)
                REQUIRE(book.bid_at_level(1).volume == 5); // next highest bid (104)
                REQUIRE(book.bid_at_level(2).volume == 20); // lowest bid (103)
            }
        }

        WHEN("A bid order quantity is set to zero")
        {
            book.update_bid(103, jazzy::tests::order{.order_id = 3, .volume = 0});

            THEN("The level should show zero volume")
            {
                REQUIRE(book.bid_volume_at_tick(105) == 10);
                REQUIRE(book.bid_volume_at_tick(104) == 15);
                REQUIRE(book.bid_volume_at_tick(103) == 0);
            }
        }
    }
}

SCENARIO("order books can have bid orders removed", "[orderbook]")
{
    GIVEN("An order book with multiple bid orders")
    {
        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        book.insert_bid(105, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(104, jazzy::tests::order{.order_id = 2, .volume = 15});
        book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 20});
        book.insert_bid(103, jazzy::tests::order{.order_id = 4, .volume = 5});

        REQUIRE(book.bid_volume_at_tick(105) == 10);
        REQUIRE(book.bid_volume_at_tick(104) == 15);
        REQUIRE(book.bid_volume_at_tick(103) == 25);

        WHEN("A single bid order is removed from a level with one order")
        {
            book.remove_bid(105, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The level should show zero volume")
            {
                REQUIRE(book.bid_volume_at_tick(105) == 0);
                REQUIRE(book.bid_volume_at_tick(104) == 15);
                REQUIRE(book.bid_volume_at_tick(103) == 25);

                REQUIRE(book.bid_at_level(0).volume == 15); // highest remaining bid (104)
                REQUIRE(book.bid_at_level(1).volume == 25); // lowest bid (103)
            }
        }

        WHEN("One bid order is removed from a level with multiple orders")
        {
            book.remove_bid(103, jazzy::tests::order{.order_id = 3, .volume = 20});

            THEN("The level should reflect the remaining volume")
            {
                REQUIRE(book.bid_volume_at_tick(105) == 10);
                REQUIRE(book.bid_volume_at_tick(104) == 15);
                REQUIRE(book.bid_volume_at_tick(103) == 5);
            }
        }

        WHEN("The last bid order is removed from a level")
        {
            book.remove_bid(104, jazzy::tests::order{.order_id = 2, .volume = 15});

            THEN("The level should show zero volume")
            {
                REQUIRE(book.bid_volume_at_tick(105) == 10);
                REQUIRE(book.bid_volume_at_tick(104) == 0);
                REQUIRE(book.bid_volume_at_tick(103) == 25);
            }
        }
    }
}

SCENARIO("order books can have ask orders modified", "[orderbook]")
{
    GIVEN("An order book with multiple ask orders")
    {
        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        book.insert_ask(121, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(122, jazzy::tests::order{.order_id = 2, .volume = 15});
        book.insert_ask(123, jazzy::tests::order{.order_id = 3, .volume = 20});

        REQUIRE(book.ask_volume_at_tick(121) == 10);
        REQUIRE(book.ask_volume_at_tick(122) == 15);
        REQUIRE(book.ask_volume_at_tick(123) == 20);

        WHEN("An ask order quantity is increased")
        {
            book.update_ask(121, jazzy::tests::order{.order_id = 1, .volume = 25});

            THEN("The level should reflect the increase")
            {
                REQUIRE(book.ask_volume_at_tick(121) == 25);
                REQUIRE(book.ask_volume_at_tick(122) == 15);
                REQUIRE(book.ask_volume_at_tick(123) == 20);

                REQUIRE(book.ask_at_level(0).volume == 25); // lowest ask (121)
                REQUIRE(book.ask_at_level(1).volume == 15); // next lowest ask (122)
                REQUIRE(book.ask_at_level(2).volume == 20); // highest ask (123)
            }
        }

        WHEN("An ask order quantity is decreased")
        {
            book.update_ask(122, jazzy::tests::order{.order_id = 2, .volume = 5});

            THEN("The level should reflect the decrease")
            {
                REQUIRE(book.ask_volume_at_tick(121) == 10);
                REQUIRE(book.ask_volume_at_tick(122) == 5);
                REQUIRE(book.ask_volume_at_tick(123) == 20);

                REQUIRE(book.ask_at_level(0).volume == 10); // lowest ask (121)
                REQUIRE(book.ask_at_level(1).volume == 5); // next lowest ask (122)
                REQUIRE(book.ask_at_level(2).volume == 20); // highest ask (123)
            }
        }

        WHEN("An ask order quantity is set to zero")
        {
            book.update_ask(123, jazzy::tests::order{.order_id = 3, .volume = 0});

            THEN("The level should show zero volume")
            {
                REQUIRE(book.ask_volume_at_tick(121) == 10);
                REQUIRE(book.ask_volume_at_tick(122) == 15);
                REQUIRE(book.ask_volume_at_tick(123) == 0);
            }
        }
    }
}

SCENARIO("order books can have ask orders removed", "[orderbook]")
{
    GIVEN("An order book with multiple ask orders")
    {
        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        book.insert_ask(121, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(122, jazzy::tests::order{.order_id = 2, .volume = 15});
        book.insert_ask(123, jazzy::tests::order{.order_id = 3, .volume = 20});
        book.insert_ask(123, jazzy::tests::order{.order_id = 4, .volume = 5});

        REQUIRE(book.ask_volume_at_tick(121) == 10);
        REQUIRE(book.ask_volume_at_tick(122) == 15);
        REQUIRE(book.ask_volume_at_tick(123) == 25);

        WHEN("A single ask order is removed from a level with one order")
        {
            book.remove_ask(121, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The level should show zero volume")
            {
                REQUIRE(book.ask_volume_at_tick(121) == 0);
                REQUIRE(book.ask_volume_at_tick(122) == 15);
                REQUIRE(book.ask_volume_at_tick(123) == 25);

                REQUIRE(book.ask_at_level(0).volume == 15); // lowest remaining ask (122)
                REQUIRE(book.ask_at_level(1).volume == 25); // highest ask (123)
            }
        }

        WHEN("One ask order is removed from a level with multiple orders")
        {
            book.remove_ask(123, jazzy::tests::order{.order_id = 3, .volume = 20});

            THEN("The level should reflect the remaining volume")
            {
                REQUIRE(book.ask_volume_at_tick(121) == 10);
                REQUIRE(book.ask_volume_at_tick(122) == 15);
                REQUIRE(book.ask_volume_at_tick(123) == 5);
            }
        }

        WHEN("The last ask order is removed from a level")
        {
            book.remove_ask(122, jazzy::tests::order{.order_id = 2, .volume = 15});

            THEN("The level should show zero volume")
            {
                REQUIRE(book.ask_volume_at_tick(121) == 10);
                REQUIRE(book.ask_volume_at_tick(122) == 0);
                REQUIRE(book.ask_volume_at_tick(123) == 25);
            }
        }
    }
}
