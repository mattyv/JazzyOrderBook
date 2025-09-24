#include "../benchmarks/map_order_book.hpp"
#include <catch.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;

SCENARIO("order books can have bid orders added", "[orderbook]")
{
    GIVEN("An empty order book")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int base = 110;
        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));

        auto test_empty_book = []<typename BookType>(BookType& book) { REQUIRE(book.size() == expected_size); };

        VectorOrderBook vector_book{};
        MapOrderBook map_book{};

        test_empty_book(vector_book);
        test_empty_book(map_book);

        WHEN("A buy side order is added")
        {
            auto test_single_bid = []<typename BookType>(BookType& book) {
                book.insert_bid(101, jazzy::tests::order{.order_id = 1, .volume = 10});
                REQUIRE(book.bid_volume_at_tick(101) == 10);
            };

            test_single_bid(vector_book);
            test_single_bid(map_book);
        }

        WHEN("multiple buy side orders are added")
        {
            auto test_multiple_bids = []<typename BookType>(BookType& book) {
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
            };

            VectorOrderBook vector_book2{};
            MapOrderBook map_book2{};
            test_multiple_bids(vector_book2);
            test_multiple_bids(map_book2);
        }
    }

    GIVEN("An order book with a number of bid orders")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int base = 110;
        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));

        auto setup_book_with_orders = []<typename BookType>(BookType& book) {
            REQUIRE(book.size() == expected_size);

            book.insert_bid(101, jazzy::tests::order{.order_id = 1, .volume = 1});
            book.insert_bid(102, jazzy::tests::order{.order_id = 2, .volume = 2});
            book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 3});

            book.insert_bid(99, jazzy::tests::order{.order_id = 4, .volume = 4});
            book.insert_bid(98, jazzy::tests::order{.order_id = 5, .volume = 5});
            book.insert_bid(97, jazzy::tests::order{.order_id = 6, .volume = 6});
            book.insert_bid(97, jazzy::tests::order{.order_id = 7, .volume = 4});
        };

        WHEN("A bids order quantity is increased")
        {
            auto test_quantity_increase = []<typename BookType>(BookType& book) {
                book.update_bid(97, jazzy::tests::order{.order_id = 6, .volume = 10});
                THEN("Then the level should reflect the adjustment")
                {
                    REQUIRE(book.bid_volume_at_tick(97) == 14);
                    REQUIRE(book.bid_at_level(5).volume == 14); // lowest bid level
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_orders(vector_book);
            setup_book_with_orders(map_book);
            test_quantity_increase(vector_book);
            test_quantity_increase(map_book);
        }

        WHEN("A bids order quantity is decreased")
        {
            auto test_quantity_decrease = []<typename BookType>(BookType& book) {
                book.update_bid(97, jazzy::tests::order{.order_id = 6, .volume = 4});
                THEN("Then the level should reflect the adjustment")
                {
                    REQUIRE(book.bid_volume_at_tick(97) == 8);
                    REQUIRE(book.bid_at_level(5).volume == 8); // lowest bid level
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_orders(vector_book);
            setup_book_with_orders(map_book);
            test_quantity_decrease(vector_book);
            test_quantity_decrease(map_book);
        }
    }

    GIVEN("An order book with a number of ask orders")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int base = 90;
        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));

        auto setup_book_with_asks = []<typename BookType>(BookType& book) {
            REQUIRE(book.size() == expected_size);

            book.insert_ask(101, jazzy::tests::order{.order_id = 1, .volume = 1});
            book.insert_ask(102, jazzy::tests::order{.order_id = 2, .volume = 2});
            book.insert_ask(103, jazzy::tests::order{.order_id = 3, .volume = 3});

            book.insert_ask(99, jazzy::tests::order{.order_id = 4, .volume = 4});
            book.insert_ask(98, jazzy::tests::order{.order_id = 5, .volume = 5});
            book.insert_ask(97, jazzy::tests::order{.order_id = 6, .volume = 6});
            book.insert_ask(97, jazzy::tests::order{.order_id = 7, .volume = 4});
        };

        WHEN("A ask order quantity is increased")
        {
            auto test_ask_increase = []<typename BookType>(BookType& book) {
                book.update_ask(97, jazzy::tests::order{.order_id = 6, .volume = 10});
                THEN("Then the level should reflect the adjustment") { REQUIRE(book.ask_volume_at_tick(97) == 14); }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_asks(vector_book);
            setup_book_with_asks(map_book);
            test_ask_increase(vector_book);
            test_ask_increase(map_book);
        }

        WHEN("A ask order quantity is decreased")
        {
            auto test_ask_decrease = []<typename BookType>(BookType& book) {
                book.update_ask(97, jazzy::tests::order{.order_id = 6, .volume = 4});
                THEN("Then the level should reflect the adjustment") { REQUIRE(book.ask_volume_at_tick(97) == 8); }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_asks(vector_book);
            setup_book_with_asks(map_book);
            test_ask_decrease(vector_book);
            test_ask_decrease(map_book);
        }
    }
}

SCENARIO("order books can have ask orders added", "[orderbook]")
{
    GIVEN("An empty order book")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int base = 110;
        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));

        auto test_empty_book = []<typename BookType>(BookType& book) { REQUIRE(book.size() == expected_size); };

        VectorOrderBook vector_book{};
        MapOrderBook map_book{};

        test_empty_book(vector_book);
        test_empty_book(map_book);

        WHEN("A sell side order is added")
        {
            auto test_single_ask = []<typename BookType>(BookType& book) {
                book.insert_ask(115, jazzy::tests::order{.order_id = 1, .volume = 10});
                REQUIRE(book.ask_volume_at_tick(115) == 10);
            };

            test_single_ask(vector_book);
            test_single_ask(map_book);
        }

        WHEN("multiple sell side orders are added")
        {
            auto test_multiple_asks = []<typename BookType>(BookType& book) {
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
            };

            VectorOrderBook vector_book2{};
            MapOrderBook map_book2{};
            test_multiple_asks(vector_book2);
            test_multiple_asks(map_book2);
        }
    }
    GIVEN("A non empty order book")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int base = 110;
        static constexpr auto expected_size = static_cast<size_t>((130 - 90) * (1.0 + 2000 / 10000.0));

        auto setup_non_empty_book = []<typename BookType>(BookType& book) {
            REQUIRE(book.size() == expected_size);

            book.insert_ask(115, jazzy::tests::order{.order_id = 1, .volume = 1});
            book.insert_ask(116, jazzy::tests::order{.order_id = 2, .volume = 2});
            book.insert_ask(117, jazzy::tests::order{.order_id = 3, .volume = 3});

            book.insert_ask(120, jazzy::tests::order{.order_id = 4, .volume = 4});
            book.insert_ask(121, jazzy::tests::order{.order_id = 5, .volume = 5});
            book.insert_ask(122, jazzy::tests::order{.order_id = 6, .volume = 6});
            book.insert_ask(122, jazzy::tests::order{.order_id = 7, .volume = 4});
        };

        WHEN("An ask orders quantity is increased")
        {
            auto test_ask_quantity_increase = []<typename BookType>(BookType& book) {
                book.update_ask(122, jazzy::tests::order{.order_id = 6, .volume = 10});
                THEN("Then the level should reflect the adjustment")
                {
                    REQUIRE(book.ask_volume_at_tick(122) == 14);
                    REQUIRE(book.ask_at_level(5).volume == 14); // highest ask level
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_non_empty_book(vector_book);
            setup_non_empty_book(map_book);
            test_ask_quantity_increase(vector_book);
            test_ask_quantity_increase(map_book);
        }
    }
}

SCENARIO("order books can have bid orders modified", "[orderbook]")
{
    GIVEN("An order book with multiple bid orders")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;

        auto setup_book_with_multiple_bids = []<typename BookType>(BookType& book) {
            book.insert_bid(105, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(104, jazzy::tests::order{.order_id = 2, .volume = 15});
            book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 20});

            REQUIRE(book.bid_volume_at_tick(105) == 10);
            REQUIRE(book.bid_volume_at_tick(104) == 15);
            REQUIRE(book.bid_volume_at_tick(103) == 20);
        };

        WHEN("A bid order quantity is increased")
        {
            auto test_bid_quantity_increase = []<typename BookType>(BookType& book) {
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
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_multiple_bids(vector_book);
            setup_book_with_multiple_bids(map_book);
            test_bid_quantity_increase(vector_book);
            test_bid_quantity_increase(map_book);
        }

        WHEN("A bid order quantity is decreased")
        {
            auto test_bid_quantity_decrease = []<typename BookType>(BookType& book) {
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
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_multiple_bids(vector_book);
            setup_book_with_multiple_bids(map_book);
            test_bid_quantity_decrease(vector_book);
            test_bid_quantity_decrease(map_book);
        }

        WHEN("A bid order quantity is set to zero")
        {
            auto test_bid_quantity_zero = []<typename BookType>(BookType& book) {
                book.update_bid(103, jazzy::tests::order{.order_id = 3, .volume = 0});

                THEN("The level should show zero volume")
                {
                    REQUIRE(book.bid_volume_at_tick(105) == 10);
                    REQUIRE(book.bid_volume_at_tick(104) == 15);
                    REQUIRE(book.bid_volume_at_tick(103) == 0);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_multiple_bids(vector_book);
            setup_book_with_multiple_bids(map_book);
            test_bid_quantity_zero(vector_book);
            test_bid_quantity_zero(map_book);
        }
    }
}

SCENARIO("order books can have bid orders removed", "[orderbook]")
{
    GIVEN("An order book with multiple bid orders")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;

        auto setup_book_for_removal = []<typename BookType>(BookType& book) {
            book.insert_bid(105, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(104, jazzy::tests::order{.order_id = 2, .volume = 15});
            book.insert_bid(103, jazzy::tests::order{.order_id = 3, .volume = 20});
            book.insert_bid(103, jazzy::tests::order{.order_id = 4, .volume = 5});

            REQUIRE(book.bid_volume_at_tick(105) == 10);
            REQUIRE(book.bid_volume_at_tick(104) == 15);
            REQUIRE(book.bid_volume_at_tick(103) == 25);
        };

        WHEN("A single bid order is removed from a level with one order")
        {
            auto test_remove_single_order = []<typename BookType>(BookType& book) {
                book.remove_bid(105, jazzy::tests::order{.order_id = 1, .volume = 10});

                THEN("The level should show zero volume")
                {
                    REQUIRE(book.bid_volume_at_tick(105) == 0);
                    REQUIRE(book.bid_volume_at_tick(104) == 15);
                    REQUIRE(book.bid_volume_at_tick(103) == 25);

                    REQUIRE(book.bid_at_level(0).volume == 15); // highest remaining bid (104)
                    REQUIRE(book.bid_at_level(1).volume == 25); // lowest bid (103)
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_for_removal(vector_book);
            setup_book_for_removal(map_book);
            test_remove_single_order(vector_book);
            test_remove_single_order(map_book);
        }

        WHEN("One bid order is removed from a level with multiple orders")
        {
            auto test_remove_from_multiple = []<typename BookType>(BookType& book) {
                book.remove_bid(103, jazzy::tests::order{.order_id = 3, .volume = 20});

                THEN("The level should reflect the remaining volume")
                {
                    REQUIRE(book.bid_volume_at_tick(105) == 10);
                    REQUIRE(book.bid_volume_at_tick(104) == 15);
                    REQUIRE(book.bid_volume_at_tick(103) == 5);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_for_removal(vector_book);
            setup_book_for_removal(map_book);
            test_remove_from_multiple(vector_book);
            test_remove_from_multiple(map_book);
        }

        WHEN("The last bid order is removed from a level")
        {
            auto test_remove_last_order = []<typename BookType>(BookType& book) {
                book.remove_bid(104, jazzy::tests::order{.order_id = 2, .volume = 15});

                THEN("The level should show zero volume")
                {
                    REQUIRE(book.bid_volume_at_tick(105) == 10);
                    REQUIRE(book.bid_volume_at_tick(104) == 0);
                    REQUIRE(book.bid_volume_at_tick(103) == 25);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_for_removal(vector_book);
            setup_book_for_removal(map_book);
            test_remove_last_order(vector_book);
            test_remove_last_order(map_book);
        }
    }
}

SCENARIO("order books can have ask orders modified", "[orderbook]")
{
    GIVEN("An order book with multiple ask orders")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;

        auto setup_book_with_multiple_asks = []<typename BookType>(BookType& book) {
            book.insert_ask(121, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(122, jazzy::tests::order{.order_id = 2, .volume = 15});
            book.insert_ask(123, jazzy::tests::order{.order_id = 3, .volume = 20});

            REQUIRE(book.ask_volume_at_tick(121) == 10);
            REQUIRE(book.ask_volume_at_tick(122) == 15);
            REQUIRE(book.ask_volume_at_tick(123) == 20);
        };

        WHEN("An ask order quantity is increased")
        {
            auto test_ask_quantity_increase = []<typename BookType>(BookType& book) {
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
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_multiple_asks(vector_book);
            setup_book_with_multiple_asks(map_book);
            test_ask_quantity_increase(vector_book);
            test_ask_quantity_increase(map_book);
        }

        WHEN("An ask order quantity is decreased")
        {
            auto test_ask_quantity_decrease = []<typename BookType>(BookType& book) {
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
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_multiple_asks(vector_book);
            setup_book_with_multiple_asks(map_book);
            test_ask_quantity_decrease(vector_book);
            test_ask_quantity_decrease(map_book);
        }

        WHEN("An ask order quantity is set to zero")
        {
            auto test_ask_quantity_zero = []<typename BookType>(BookType& book) {
                book.update_ask(123, jazzy::tests::order{.order_id = 3, .volume = 0});

                THEN("The level should show zero volume")
                {
                    REQUIRE(book.ask_volume_at_tick(121) == 10);
                    REQUIRE(book.ask_volume_at_tick(122) == 15);
                    REQUIRE(book.ask_volume_at_tick(123) == 0);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_multiple_asks(vector_book);
            setup_book_with_multiple_asks(map_book);
            test_ask_quantity_zero(vector_book);
            test_ask_quantity_zero(map_book);
        }
    }
}

SCENARIO("order books can have ask orders removed", "[orderbook]")
{
    GIVEN("An order book with multiple ask orders")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        static constexpr int bid_base = 110;
        static constexpr int ask_base = 120;

        auto setup_book_for_ask_removal = []<typename BookType>(BookType& book) {
            book.insert_ask(121, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(122, jazzy::tests::order{.order_id = 2, .volume = 15});
            book.insert_ask(123, jazzy::tests::order{.order_id = 3, .volume = 20});
            book.insert_ask(123, jazzy::tests::order{.order_id = 4, .volume = 5});

            REQUIRE(book.ask_volume_at_tick(121) == 10);
            REQUIRE(book.ask_volume_at_tick(122) == 15);
            REQUIRE(book.ask_volume_at_tick(123) == 25);
        };

        WHEN("A single ask order is removed from a level with one order")
        {
            auto test_remove_single_ask = []<typename BookType>(BookType& book) {
                book.remove_ask(121, jazzy::tests::order{.order_id = 1, .volume = 10});

                THEN("The level should show zero volume")
                {
                    REQUIRE(book.ask_volume_at_tick(121) == 0);
                    REQUIRE(book.ask_volume_at_tick(122) == 15);
                    REQUIRE(book.ask_volume_at_tick(123) == 25);

                    REQUIRE(book.ask_at_level(0).volume == 15); // lowest remaining ask (122)
                    REQUIRE(book.ask_at_level(1).volume == 25); // highest ask (123)
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_for_ask_removal(vector_book);
            setup_book_for_ask_removal(map_book);
            test_remove_single_ask(vector_book);
            test_remove_single_ask(map_book);
        }

        WHEN("One ask order is removed from a level with multiple orders")
        {
            auto test_remove_from_multiple_asks = []<typename BookType>(BookType& book) {
                book.remove_ask(123, jazzy::tests::order{.order_id = 3, .volume = 20});

                THEN("The level should reflect the remaining volume")
                {
                    REQUIRE(book.ask_volume_at_tick(121) == 10);
                    REQUIRE(book.ask_volume_at_tick(122) == 15);
                    REQUIRE(book.ask_volume_at_tick(123) == 5);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_for_ask_removal(vector_book);
            setup_book_for_ask_removal(map_book);
            test_remove_from_multiple_asks(vector_book);
            test_remove_from_multiple_asks(map_book);
        }

        WHEN("The last ask order is removed from a level")
        {
            auto test_remove_last_ask = []<typename BookType>(BookType& book) {
                book.remove_ask(122, jazzy::tests::order{.order_id = 2, .volume = 15});

                THEN("The level should show zero volume")
                {
                    REQUIRE(book.ask_volume_at_tick(121) == 10);
                    REQUIRE(book.ask_volume_at_tick(122) == 0);
                    REQUIRE(book.ask_volume_at_tick(123) == 25);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_for_ask_removal(vector_book);
            setup_book_for_ask_removal(map_book);
            test_remove_last_ask(vector_book);
            test_remove_last_ask(map_book);
        }
    }
}

SCENARIO("order book edge cases and comprehensive testing", "[orderbook][edge_cases]")
{
    using edge_market_stats = jazzy::market_statistics<int, 150, 50, 100, 1000>;

    using VectorOrderBookEdge = jazzy::order_book<int, jazzy::tests::order, edge_market_stats>;
    using MapOrderBookEdge = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, edge_market_stats>;

    GIVEN("An empty order book")
    {
        WHEN("Querying levels on empty book")
        {
            auto test_empty_book_levels = []<typename BookType>(BookType& book) {
                REQUIRE(book.bid_at_level(0).volume == 0);
                REQUIRE(book.bid_at_level(5).volume == 0);
                REQUIRE(book.ask_at_level(0).volume == 0);
                REQUIRE(book.ask_at_level(5).volume == 0);
            };

            VectorOrderBookEdge vector_book{};
            MapOrderBookEdge map_book{};
            test_empty_book_levels(vector_book);
            test_empty_book_levels(map_book);
        }
    }

    GIVEN("A book with bids at boundary tick values")
    {
        WHEN("Adding bids at daily_high and near daily_low")
        {
            auto test_boundary_bids = []<typename BookType>(BookType& book) {
                book.insert_bid(150, jazzy::tests::order{.order_id = 1, .volume = 100}); // daily_high
                book.insert_bid(51, jazzy::tests::order{.order_id = 2, .volume = 200}); // near daily_low
                book.insert_bid(52, jazzy::tests::order{.order_id = 3, .volume = 300});

                REQUIRE(book.bid_at_level(0).volume == 100); // highest bid (150)
                REQUIRE(book.bid_at_level(1).volume == 300); // next highest (52)
                REQUIRE(book.bid_at_level(2).volume == 200); // lowest bid (51)

                REQUIRE(book.bid_volume_at_tick(150) == 100);
                REQUIRE(book.bid_volume_at_tick(52) == 300);
                REQUIRE(book.bid_volume_at_tick(51) == 200);
            };

            VectorOrderBookEdge vector_book{};
            MapOrderBookEdge map_book{};
            test_boundary_bids(vector_book);
            test_boundary_bids(map_book);
        }
    }

    GIVEN("A book with asks at boundary tick values")
    {
        WHEN("Adding asks at daily_low and near daily_high")
        {
            auto test_boundary_asks = []<typename BookType>(BookType& book) {
                book.insert_ask(50, jazzy::tests::order{.order_id = 1, .volume = 100}); // daily_low
                book.insert_ask(149, jazzy::tests::order{.order_id = 2, .volume = 200}); // near daily_high
                book.insert_ask(148, jazzy::tests::order{.order_id = 3, .volume = 300});

                REQUIRE(book.ask_at_level(0).volume == 100); // lowest ask (50)
                REQUIRE(book.ask_at_level(1).volume == 300); // next lowest (148)
                REQUIRE(book.ask_at_level(2).volume == 200); // highest ask (149)

                REQUIRE(book.ask_volume_at_tick(50) == 100);
                REQUIRE(book.ask_volume_at_tick(148) == 300);
                REQUIRE(book.ask_volume_at_tick(149) == 200);
            };

            VectorOrderBookEdge vector_book{};
            MapOrderBookEdge map_book{};
            test_boundary_asks(vector_book);
            test_boundary_asks(map_book);
        }
    }
}

SCENARIO("order book best bid/ask optimization", "[orderbook][performance]")
{
    GIVEN("A book with multiple bid levels")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        auto setup_book_with_bid_levels = []<typename BookType>(BookType& book) {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(99, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_bid(98, jazzy::tests::order{.order_id = 3, .volume = 30});
        };

        WHEN("Updating volume without changing tick (should not trigger rescan)")
        {
            auto test_volume_update = []<typename BookType>(BookType& book) {
                book.update_bid(99, jazzy::tests::order{.order_id = 2, .volume = 25});

                THEN("Levels should reflect the change")
                {
                    REQUIRE(book.bid_at_level(0).volume == 10); // best bid unchanged
                    REQUIRE(book.bid_at_level(1).volume == 25); // updated volume
                    REQUIRE(book.bid_at_level(2).volume == 30);
                    REQUIRE(book.bid_volume_at_tick(99) == 25);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_bid_levels(vector_book);
            setup_book_with_bid_levels(map_book);
            test_volume_update(vector_book);
            test_volume_update(map_book);
        }

        WHEN("Completely removing the best bid level")
        {
            auto test_best_bid_removal = []<typename BookType>(BookType& book) {
                book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 0});

                THEN("Best bid should update to next level")
                {
                    REQUIRE(book.bid_at_level(0).volume == 20); // 99 is now best
                    REQUIRE(book.bid_at_level(1).volume == 30); // 98 is next
                    REQUIRE(book.bid_volume_at_tick(100) == 0);
                    REQUIRE(book.bid_volume_at_tick(99) == 20);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_bid_levels(vector_book);
            setup_book_with_bid_levels(map_book);
            test_best_bid_removal(vector_book);
            test_best_bid_removal(map_book);
        }

        WHEN("Moving an order to a better price")
        {
            auto test_move_to_better_price = []<typename BookType>(BookType& book) {
                book.update_bid(101, jazzy::tests::order{.order_id = 2, .volume = 20}); // move from 99 to 101

                THEN("Best bid should update immediately")
                {
                    REQUIRE(book.bid_at_level(0).volume == 20); // 101 is now best
                    REQUIRE(book.bid_at_level(1).volume == 10); // 100 is next
                    REQUIRE(book.bid_at_level(2).volume == 30); // 98 remains
                    REQUIRE(book.bid_volume_at_tick(101) == 20);
                    REQUIRE(book.bid_volume_at_tick(99) == 0);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_bid_levels(vector_book);
            setup_book_with_bid_levels(map_book);
            test_move_to_better_price(vector_book);
            test_move_to_better_price(map_book);
        }
    }

    GIVEN("A book with multiple ask levels")
    {
        using VectorOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats>;
        using MapOrderBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, test_market_stats>;

        auto setup_book_with_ask_levels = []<typename BookType>(BookType& book) {
            book.insert_ask(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(101, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_ask(102, jazzy::tests::order{.order_id = 3, .volume = 30});
        };

        WHEN("Completely removing the best ask level")
        {
            auto test_best_ask_removal = []<typename BookType>(BookType& book) {
                book.update_ask(100, jazzy::tests::order{.order_id = 1, .volume = 0});

                THEN("Best ask should update to next level")
                {
                    REQUIRE(book.ask_at_level(0).volume == 20); // 101 is now best
                    REQUIRE(book.ask_at_level(1).volume == 30); // 102 is next
                    REQUIRE(book.ask_volume_at_tick(100) == 0);
                    REQUIRE(book.ask_volume_at_tick(101) == 20);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_ask_levels(vector_book);
            setup_book_with_ask_levels(map_book);
            test_best_ask_removal(vector_book);
            test_best_ask_removal(map_book);
        }

        WHEN("Moving an order to a better price")
        {
            auto test_move_ask_to_better_price = []<typename BookType>(BookType& book) {
                book.update_ask(99, jazzy::tests::order{.order_id = 2, .volume = 20}); // move from 101 to 99

                THEN("Best ask should update immediately")
                {
                    REQUIRE(book.ask_at_level(0).volume == 20); // 99 is now best
                    REQUIRE(book.ask_at_level(1).volume == 10); // 100 is next
                    REQUIRE(book.ask_at_level(2).volume == 30); // 102 remains
                    REQUIRE(book.ask_volume_at_tick(99) == 20);
                    REQUIRE(book.ask_volume_at_tick(101) == 0);
                }
            };

            VectorOrderBook vector_book{};
            MapOrderBook map_book{};
            setup_book_with_ask_levels(vector_book);
            setup_book_with_ask_levels(map_book);
            test_move_ask_to_better_price(vector_book);
            test_move_ask_to_better_price(map_book);
        }
    }
}

SCENARIO("order book bitmap functionality", "[orderbook][bitmap]")
{
    GIVEN("An empty order book")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        THEN("All bitmap bits should initially be false")
        {
            REQUIRE(book.bid_bitmap().none());
            REQUIRE(book.ask_bitmap().none());
        }

        WHEN("Adding bid orders")
        {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(99, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_bid(98, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("Corresponding bitmap bits should be set")
            {
                REQUIRE(!book.bid_bitmap().none());
                REQUIRE(book.ask_bitmap().none());

                // Check specific bits are set (we can't know exact indices without the conversion logic)
                REQUIRE(book.bid_bitmap().count() == 3);
            }
        }

        WHEN("Adding ask orders")
        {
            book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(111, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_ask(112, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("Corresponding ask bitmap bits should be set")
            {
                REQUIRE(book.bid_bitmap().none());
                REQUIRE(!book.ask_bitmap().none());

                REQUIRE(book.ask_bitmap().count() == 3);
            }
        }
    }

    GIVEN("A book with existing orders")
    {
        jazzy::order_book<int, jazzy::tests::order, test_market_stats> book{};

        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(99, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_ask(110, jazzy::tests::order{.order_id = 3, .volume = 15});

        WHEN("Updating an order to zero volume at the best bid")
        {
            auto initial_bid_count = book.bid_bitmap().count();
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 0});

            THEN("The bitmap should reflect the removed level")
            {
                REQUIRE(book.bid_bitmap().count() == initial_bid_count - 1);
                REQUIRE(book.bid_volume_at_tick(100) == 0);
            }
        }

        WHEN("Updating an order to zero volume at the best ask")
        {
            auto initial_ask_count = book.ask_bitmap().count();
            book.update_ask(110, jazzy::tests::order{.order_id = 3, .volume = 0});

            THEN("The bitmap should reflect the removed level")
            {
                REQUIRE(book.ask_bitmap().count() == initial_ask_count - 1);
                REQUIRE(book.ask_volume_at_tick(110) == 0);
            }
        }

        WHEN("Moving an order to a different price level")
        {
            auto initial_bid_count = book.bid_bitmap().count();
            book.update_bid(101, jazzy::tests::order{.order_id = 1, .volume = 10}); // move from 100 to 101

            THEN("Old level bitmap should be cleared if no volume remains, new level should be set")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 0);
                REQUIRE(book.bid_volume_at_tick(101) == 10);
                // Total count should remain the same since we moved from one level to another
                REQUIRE(book.bid_bitmap().count() == initial_bid_count);
            }
        }
    }
}

SCENARIO("order book market stats sizing", "[orderbook][sizing]")
{
    GIVEN("Different market statistics configurations")
    {
        using small_market = jazzy::market_statistics<int, 110, 90, 100, 500>;
        using large_market = jazzy::market_statistics<int, 200, 50, 125, 3000>;

        using VectorSmallBook = jazzy::order_book<int, jazzy::tests::order, small_market>;
        using VectorLargeBook = jazzy::order_book<int, jazzy::tests::order, large_market>;
        using MapSmallBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, small_market>;
        using MapLargeBook = jazzy::benchmarks::map_order_book<int, jazzy::tests::order, large_market>;

        auto test_book_sizes = []<typename SmallBookType, typename LargeBookType>(
                                   SmallBookType& small_book, LargeBookType& large_book) {
            // small: (110-90) * (1 + 0.05) = 20 * 1.05 = 21
            REQUIRE(small_book.size() == 21);

            // large: (200-50) * (1 + 0.30) = 150 * 1.30 = 195
            REQUIRE(large_book.size() == 195);
        };

        VectorSmallBook vector_small_book{};
        VectorLargeBook vector_large_book{};
        MapSmallBook map_small_book{};
        MapLargeBook map_large_book{};

        test_book_sizes(vector_small_book, vector_large_book);
        test_book_sizes(map_small_book, map_large_book);

        WHEN("Adding orders to both books")
        {
            auto test_order_handling = []<typename SmallBookType, typename LargeBookType>(
                                           SmallBookType& small_book, LargeBookType& large_book) {
                small_book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
                large_book.insert_bid(150, jazzy::tests::order{.order_id = 1, .volume = 20});

                REQUIRE(small_book.bid_volume_at_tick(100) == 10);
                REQUIRE(large_book.bid_volume_at_tick(150) == 20);
            };

            VectorSmallBook vector_small_book{};
            VectorLargeBook vector_large_book{};
            MapSmallBook map_small_book{};
            MapLargeBook map_large_book{};

            test_order_handling(vector_small_book, vector_large_book);
            test_order_handling(map_small_book, map_large_book);
        }
    }
}
