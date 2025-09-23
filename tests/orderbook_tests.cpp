#include "catch.hpp"
#include <jazzy/order_book.hpp>

namespace jazzy::tests {
struct order
{
    int order_id{};
    size_t volume{};
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

        WHEN("An order is added")
        {
            THEN("Then the level should reflect the quantity") {}
        }
    }
}
