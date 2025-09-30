#include <catch.hpp>

#define JAZZY_HAS_BUILTIN_POPCOUNT 0
#define JAZZY_HAS_BUILTIN_CTZ 0
#define JAZZY_HAS_BUILTIN_CLZ 0
#include <jazzy/detail/level_bitmap.hpp>

TEST_CASE("level_bitmap portable path", "[level_bitmap][portable]")
{
    using bitmap = jazzy::detail::level_bitmap<64>;
    bitmap bm;

    bm.set(0, true);
    bm.set(1, true);
    bm.set(63, true);

    REQUIRE(bm.count() == 3);
    REQUIRE(bm.select_from_low(0) == 0);
    REQUIRE(bm.select_from_low(1) == 1);
    REQUIRE(bm.select_from_low(2) == 63);
    REQUIRE(bm.select_from_high(0) == 63);
    REQUIRE(bm.select_from_high(1) == 1);
    REQUIRE(bm.select_from_high(2) == 0);
    REQUIRE(bm.find_lowest() == 0);
    REQUIRE(bm.find_highest() == 63);

    bm.set(1, false);
    REQUIRE(bm.count() == 2);
    REQUIRE(bm.select_from_low(1) == 63);
    REQUIRE(bm.find_highest() == 63);
}
