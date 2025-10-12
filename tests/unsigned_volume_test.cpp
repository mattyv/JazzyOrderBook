#include <catch.hpp>
#include <jazzy/order_book.hpp>

namespace jazzy::tests {

// Test order with unsigned volume type
struct unsigned_order
{
    int order_id{};
    unsigned int volume{}; // Unsigned volume
    int tick{};
};

inline int jazzy_order_id_getter(unsigned_order const& o) { return o.order_id; }
inline unsigned int jazzy_order_volume_getter(unsigned_order const& o) { return o.volume; }
inline int jazzy_order_tick_getter(unsigned_order const& o) { return o.tick; }
inline void jazzy_order_volume_setter(unsigned_order& o, unsigned int volume) { o.volume = volume; }
inline void jazzy_order_tick_setter(unsigned_order& o, int tick) { o.tick = tick; }

} // namespace jazzy::tests

using test_market_stats = jazzy::market_statistics<int, 130, 90, 110, 2000>;
using UnsignedOrderBook = jazzy::order_book<int, jazzy::tests::unsigned_order, test_market_stats>;

SCENARIO("Order book correctly handles unsigned volume types", "[orderbook][unsigned]")
{
    GIVEN("An order book with unsigned volume orders")
    {
        UnsignedOrderBook book{};

        WHEN("An order volume is decreased")
        {
            book.insert_bid(100, jazzy::tests::unsigned_order{.order_id = 1, .volume = 100});
            book.update_bid(100, jazzy::tests::unsigned_order{.order_id = 1, .volume = 50});

            THEN("The volume should be correctly updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 50);
            }
        }

        WHEN("An order volume is increased")
        {
            book.insert_bid(100, jazzy::tests::unsigned_order{.order_id = 2, .volume = 50});
            book.update_bid(100, jazzy::tests::unsigned_order{.order_id = 2, .volume = 150});

            THEN("The volume should be correctly updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 150);
            }
        }

        WHEN("Multiple orders exist and one is decreased")
        {
            book.insert_bid(100, jazzy::tests::unsigned_order{.order_id = 3, .volume = 100});
            book.insert_bid(100, jazzy::tests::unsigned_order{.order_id = 4, .volume = 200});

            REQUIRE(book.bid_volume_at_tick(100) == 300);

            book.update_bid(100, jazzy::tests::unsigned_order{.order_id = 3, .volume = 50});

            THEN("The total volume should be correctly updated")
            {
                REQUIRE(book.bid_volume_at_tick(100) == 250);
            }
        }
    }
}
