#include "../benchmarks/fifo_map_order_book.hpp"
#include <catch.hpp>
#include <jazzy/detail/level_storage.hpp>
#include <jazzy/order_book.hpp>
#include <order.hpp>

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;
using FifoMapOrderBook = jazzy::benchmarks::fifo_map_order_book<int, jazzy::tests::order, test_market_stats>;
using fifo_storage = jazzy::detail::fifo_level_storage<jazzy::tests::order>;
using FifoOrderBook = jazzy::order_book<int, jazzy::tests::order, test_market_stats, fifo_storage>;

SCENARIO("FIFO map order book maintains queue order", "[fifo_map_orderbook]")
{
    GIVEN("An empty FIFO map order book")
    {
        FifoMapOrderBook book{};

        WHEN("Multiple orders are added to the same bid level")
        {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The front order should be the first one added")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 10);
            }

            THEN("The level volume should be the sum of all orders")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 60);
            }
        }

        WHEN("Multiple orders are added to the same ask level")
        {
            book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_ask(110, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The front order should be the first one added")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 10);
            }

            THEN("The level volume should be the sum of all orders")
            {
                REQUIRE(book.ask_volume_at_tick(110) == 60);
            }
        }
    }
}

SCENARIO("FIFO map order book aligns with Jazzy FIFO order book behaviour", "[fifo_map_orderbook][parity]")
{
    auto make_order = [](int id, int volume, int tick) {
        return jazzy::tests::order{.order_id = id, .volume = volume, .tick = tick};
    };

    GIVEN("Both books receive the same sequence of operations")
    {
        FifoMapOrderBook map_book{};
        FifoOrderBook jazzy_book{};

        auto insert_bid = [&](int tick, const jazzy::tests::order& order) {
            map_book.insert_bid(tick, order);
            jazzy_book.insert_bid(tick, order);
        };
        auto insert_ask = [&](int tick, const jazzy::tests::order& order) {
            map_book.insert_ask(tick, order);
            jazzy_book.insert_ask(tick, order);
        };
        auto update_bid = [&](int tick, const jazzy::tests::order& order) {
            map_book.update_bid(tick, order);
            jazzy_book.update_bid(tick, order);
        };
        auto update_ask = [&](int tick, const jazzy::tests::order& order) {
            map_book.update_ask(tick, order);
            jazzy_book.update_ask(tick, order);
        };
        auto remove_bid = [&](int tick, const jazzy::tests::order& order) {
            map_book.remove_bid(tick, order);
            jazzy_book.remove_bid(tick, order);
        };
        auto remove_ask = [&](int tick, const jazzy::tests::order& order) {
            map_book.remove_ask(tick, order);
            jazzy_book.remove_ask(tick, order);
        };

        AND_WHEN("Orders are inserted on both bid and ask sides")
        {
            insert_bid(100, make_order(1, 10, 100));
            insert_bid(100, make_order(2, 15, 100));
            insert_bid(101, make_order(3, 20, 101));

            insert_ask(102, make_order(4, 12, 102));
            insert_ask(102, make_order(5, 18, 102));
            insert_ask(103, make_order(6, 25, 103));

            THEN("Front-of-queue orders and volumes stay in sync")
            {
                auto map_front_bid = map_book.front_order_at_bid_level(0);
                auto jazzy_front_bid = jazzy_book.front_order_at_bid_level(0);
                REQUIRE(map_front_bid.order_id == jazzy_front_bid.order_id);
                REQUIRE(map_front_bid.volume == jazzy_front_bid.volume);
                REQUIRE(map_front_bid.tick == jazzy_front_bid.tick);

                auto map_front_ask = map_book.front_order_at_ask_level(0);
                auto jazzy_front_ask = jazzy_book.front_order_at_ask_level(0);
                REQUIRE(map_front_ask.order_id == jazzy_front_ask.order_id);
                REQUIRE(map_front_ask.volume == jazzy_front_ask.volume);
                REQUIRE(map_front_ask.tick == jazzy_front_ask.tick);

                REQUIRE(map_book.bid_volume_at_tick(100) == jazzy_book.bid_volume_at_tick(100));
                REQUIRE(map_book.ask_volume_at_tick(102) == jazzy_book.ask_volume_at_tick(102));
            }

            AND_WHEN("Volumes are updated and priorities adjust")
            {
                update_bid(100, make_order(1, 25, 100)); // increase -> move to back
                update_ask(102, make_order(4, 6, 102));  // decrease -> stay in place

                THEN("Updated queues match across implementations")
                {
                    auto map_best_bid = map_book.front_order_at_bid_level(0);
                    auto jazzy_best_bid = jazzy_book.front_order_at_bid_level(0);
                    REQUIRE(map_best_bid.order_id == jazzy_best_bid.order_id);
                    REQUIRE(map_best_bid.volume == jazzy_best_bid.volume);
                    REQUIRE(map_best_bid.tick == jazzy_best_bid.tick);

                    auto map_second_bid = map_book.front_order_at_bid_level(1);
                    auto jazzy_second_bid = jazzy_book.front_order_at_bid_level(1);
                    REQUIRE(map_second_bid.order_id == jazzy_second_bid.order_id);
                    REQUIRE(map_second_bid.volume == jazzy_second_bid.volume);
                    REQUIRE(map_second_bid.tick == jazzy_second_bid.tick);

                    auto map_best_ask = map_book.front_order_at_ask_level(0);
                    auto jazzy_best_ask = jazzy_book.front_order_at_ask_level(0);
                    REQUIRE(map_best_ask.order_id == jazzy_best_ask.order_id);
                    REQUIRE(map_best_ask.volume == jazzy_best_ask.volume);
                    REQUIRE(map_best_ask.tick == jazzy_best_ask.tick);
                }

                AND_WHEN("An order moves to a new price level")
                {
                    update_bid(102, make_order(2, 15, 102)); // move order 2 to better price
                    update_ask(101, make_order(5, 18, 101)); // move ask order 5 lower

                    THEN("Best levels and queue heads still align")
                    {
                        auto map_front_bid = map_book.front_order_at_bid_level(0);
                        auto jazzy_front_bid = jazzy_book.front_order_at_bid_level(0);
                        REQUIRE(map_front_bid.order_id == jazzy_front_bid.order_id);
                        REQUIRE(map_front_bid.tick == jazzy_front_bid.tick);

                        auto map_front_ask = map_book.front_order_at_ask_level(0);
                        auto jazzy_front_ask = jazzy_book.front_order_at_ask_level(0);
                        REQUIRE(map_front_ask.order_id == jazzy_front_ask.order_id);
                        REQUIRE(map_front_ask.tick == jazzy_front_ask.tick);

                        REQUIRE(map_book.best_bid() == jazzy_book.best_bid());
                        REQUIRE(map_book.best_ask() == jazzy_book.best_ask());
                    }

                    AND_WHEN("Orders are removed from both sides")
                    {
                        remove_bid(101, make_order(3, 20, 101));
                        remove_ask(103, make_order(6, 25, 103));

                        THEN("Remaining queue state remains equivalent")
                        {
                            REQUIRE(map_book.bid_volume_at_tick(102) == jazzy_book.bid_volume_at_tick(102));
                            REQUIRE(map_book.ask_volume_at_tick(101) == jazzy_book.ask_volume_at_tick(101));

                            auto map_bid_front = map_book.front_order_at_bid_level(0);
                            auto jazzy_bid_front = jazzy_book.front_order_at_bid_level(0);
                            REQUIRE(map_bid_front.order_id == jazzy_bid_front.order_id);

                            auto map_ask_front = map_book.front_order_at_ask_level(0);
                            auto jazzy_ask_front = jazzy_book.front_order_at_ask_level(0);
                            REQUIRE(map_ask_front.order_id == jazzy_ask_front.order_id);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("FIFO map order book handles volume updates correctly", "[fifo_map_orderbook]")
{
    GIVEN("A FIFO map order book with multiple orders at the same level")
    {
        FifoMapOrderBook book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("An order's volume is decreased")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 5});

            THEN("The order should keep its position in the queue")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 5);
            }

            THEN("The level volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 55);
            }
        }

        WHEN("An order's volume is increased")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 15});

            THEN("The order should lose priority and move to the back")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }

            THEN("The level volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 65);
            }
        }

        WHEN("An order is updated to have zero volume")
        {
            book.update_bid(100, jazzy::tests::order{.order_id = 1, .volume = 0});

            THEN("The order should be removed from the queue")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }

            THEN("The level volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 50);
            }
        }
    }

    GIVEN("A FIFO map order book with multiple ask orders at the same level")
    {
        FifoMapOrderBook book{};
        book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_ask(110, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("An order's volume is decreased")
        {
            book.update_ask(110, jazzy::tests::order{.order_id = 1, .volume = 5});

            THEN("The order should keep its position in the queue")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 5);
            }
        }

        WHEN("An order's volume is increased")
        {
            book.update_ask(110, jazzy::tests::order{.order_id = 1, .volume = 15});

            THEN("The order should lose priority and move to the back")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }
        }
    }
}

SCENARIO("FIFO map order book handles price changes", "[fifo_map_orderbook]")
{
    GIVEN("A FIFO map order book with orders at different levels")
    {
        FifoMapOrderBook book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_bid(101, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("An order is updated to a different price")
        {
            book.update_bid(101, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The order should be removed from the old level")
            {
                auto front_100 = book.front_order_at_bid_level(1);
                REQUIRE(front_100.order_id == 2);
                REQUIRE(book.bid_volume_at_tick(100) == 20);
            }

            THEN("The order should be added to the back of the new level")
            {
                auto front_101 = book.front_order_at_bid_level(0);
                REQUIRE(front_101.order_id == 3);
                REQUIRE(book.bid_volume_at_tick(101) == 40);
            }
        }
    }

    GIVEN("A FIFO map order book with ask orders at different levels")
    {
        FifoMapOrderBook book{};
        book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_ask(109, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("An order is updated to a different price")
        {
            book.update_ask(109, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The order should be removed from the old level")
            {
                auto front_110 = book.front_order_at_ask_level(1);
                REQUIRE(front_110.order_id == 2);
                REQUIRE(book.ask_volume_at_tick(110) == 20);
            }

            THEN("The order should be added to the back of the new level")
            {
                auto front_109 = book.front_order_at_ask_level(0);
                REQUIRE(front_109.order_id == 3);
                REQUIRE(book.ask_volume_at_tick(109) == 40);
            }
        }
    }
}

SCENARIO("FIFO map order book handles order removal", "[fifo_map_orderbook]")
{
    GIVEN("A FIFO map order book with multiple bid orders")
    {
        FifoMapOrderBook book{};
        book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("The front order is removed")
        {
            book.remove_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The next order should become the front")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }

            THEN("The level volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 50);
            }
        }

        WHEN("A middle order is removed")
        {
            book.remove_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The queue order should be maintained")
            {
                auto front = book.front_order_at_bid_level(0);
                REQUIRE(front.order_id == 1);
                REQUIRE(front.volume == 10);
            }

            THEN("The level volume should be updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 40);
            }
        }

        WHEN("All orders at a level are removed")
        {
            book.remove_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.remove_bid(100, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.remove_bid(100, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The level should have zero volume")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 0);
            }
        }
    }

    GIVEN("A FIFO map order book with multiple ask orders")
    {
        FifoMapOrderBook book{};
        book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
        book.insert_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
        book.insert_ask(110, jazzy::tests::order{.order_id = 3, .volume = 30});

        WHEN("The front order is removed")
        {
            book.remove_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});

            THEN("The next order should become the front")
            {
                auto front = book.front_order_at_ask_level(0);
                REQUIRE(front.order_id == 2);
                REQUIRE(front.volume == 20);
            }
        }

        WHEN("All orders at a level are removed")
        {
            book.remove_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.remove_ask(110, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.remove_ask(110, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The level should have zero volume")
            {
                REQUIRE(book.ask_volume_at_tick(110) == 0);
            }
        }
    }
}

SCENARIO("FIFO map order book handles best bid/ask tracking", "[fifo_map_orderbook]")
{
    GIVEN("A FIFO map order book with bid and ask orders")
    {
        FifoMapOrderBook book{};

        WHEN("Bid orders are added at different levels")
        {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(101, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_bid(99, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The best bid should be tracked correctly")
            {
                REQUIRE(book.best_bid() == 101);
            }
        }

        WHEN("Ask orders are added at different levels")
        {
            book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(109, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.insert_ask(111, jazzy::tests::order{.order_id = 3, .volume = 30});

            THEN("The best ask should be tracked correctly")
            {
                REQUIRE(book.best_ask() == 109);
            }
        }

        WHEN("The best bid is removed")
        {
            book.insert_bid(100, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_bid(101, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.remove_bid(101, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The best bid should be updated")
            {
                REQUIRE(book.best_bid() == 100);
            }
        }

        WHEN("The best ask is removed")
        {
            book.insert_ask(110, jazzy::tests::order{.order_id = 1, .volume = 10});
            book.insert_ask(109, jazzy::tests::order{.order_id = 2, .volume = 20});
            book.remove_ask(109, jazzy::tests::order{.order_id = 2, .volume = 20});

            THEN("The best ask should be updated")
            {
                REQUIRE(book.best_ask() == 110);
            }
        }
    }
}
