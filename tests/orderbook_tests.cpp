#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

SCENARIO("order books can have bid orders added", "[orderbook]")
{
    GIVEN("An empty order book")
    {
        static constexpr int base = 110;
        static constexpr int size = 20;
        jazzy::order_book<int, jazzy::tests::order, size> book{base, 0};

        REQUIRE(book.size() == size);
        REQUIRE(book.bidBaseValue() == base);

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
            }
        }
    }

    GIVEN("An order book with a number of bid orders")
    {
        static constexpr int base = 110;
        static constexpr int size = 20;
        jazzy::order_book<int, jazzy::tests::order, size> book{base, 0};

        REQUIRE(book.size() == size);
        REQUIRE(book.bidBaseValue() == base);

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
            THEN("Then the level should reflect the adjustment") { REQUIRE(book.bid_volume_at_tick(97) == 14); }
        }

        WHEN("A bids order quantity is decreased")
        {
            book.update_bid(97, jazzy::tests::order{.order_id = 6, .volume = 4});
            THEN("Then the level should reflect the adjustment") { REQUIRE(book.bid_volume_at_tick(97) == 8); }
        }
    }

    GIVEN("An order book with a number of ask orders")
    {
        static constexpr int base = 100;
        static constexpr int size = 20;
        jazzy::order_book<int, jazzy::tests::order, size> book{base, 0};

        REQUIRE(book.size() == size);
        REQUIRE(book.base() == base);

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
        static constexpr int size = 20;
        jazzy::order_book<int, jazzy::tests::order, size> book{0, base};

        REQUIRE(book.size() == size);
        REQUIRE(book.askBaseValue() == base);

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
            }
        }
    }
    GIVEN("A non empty order book")
    {
        static constexpr int base = 110;
        static constexpr int size = 20;
        jazzy::order_book<int, jazzy::tests::order, size> book{0, base};

        REQUIRE(book.size() == size);
        REQUIRE(book.askBaseValue() == base);

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
            THEN("Then the level should reflect the adjustment") { REQUIRE(book.ask_volume_at_tick(122) == 14); }
        }
    }
}
