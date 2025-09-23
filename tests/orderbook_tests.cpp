#include "catch.hpp"
#include <jazzy/order_book.hpp>

namespace jazzy::tests {
struct order
{
    int order_id{};
    int volume{};
};

int jazzy_order_id_getter(order const& o) { return o.order_id; }
int jazzy_order_volume_getter(order const& o) { return o.volume; }

} // namespace jazzy::tests

SCENARIO("order books can have orders added", "[orderbook]")
{
    GIVEN("An empty order book")
    {
        static constexpr int base = 100;
        static constexpr int size = 20;
        jazzy::order_book<int, jazzy::tests::order> book{base, size};

        REQUIRE(book.size() == size);
        REQUIRE(book.base() == base);

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
}
