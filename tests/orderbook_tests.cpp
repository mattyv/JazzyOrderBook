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

TEST_CASE("Constructing a orderbook")
{
    jazzy::order_book<jazzy::tests::order> ob;

    jazzy::tests::order o{.order_id = 1, .volume = 2};
    ob.insert(o);
}
