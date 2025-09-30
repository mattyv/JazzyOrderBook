#include <catch.hpp>

#include <jazzy/detail/level_bitmap.hpp>

TEST_CASE("level_bitmap basic operations", "[level_bitmap]")
{
    using bitmap = jazzy::detail::level_bitmap<128>;
    bitmap bm;
    REQUIRE(bm.none());
    REQUIRE(bm.count() == 0);

    bm.set(5, true);
    bm.set(64, true);
    bm.set(127, true);

    REQUIRE(bm.test(5));
    REQUIRE(bm.test(64));
    REQUIRE(bm.test(127));
    REQUIRE(bm.count() == 3);
    REQUIRE_FALSE(bm.none());
    REQUIRE(bm.find_lowest() == 5);
    REQUIRE(bm.find_highest() == 127);

    bm.set(64, false);
    REQUIRE_FALSE(bm.test(64));
    REQUIRE(bm.count() == 2);
    REQUIRE(bm.find_highest() == 127);

    bm.set(5, false);
    bm.set(127, false);
    REQUIRE(bm.none());
    REQUIRE(bm.find_lowest() == -1);
    REQUIRE(bm.find_highest() == -1);
}

TEST_CASE("level_bitmap select low high", "[level_bitmap]")
{
    using bitmap = jazzy::detail::level_bitmap<192>;
    bitmap bm;

    bm.set(3, true);
    bm.set(65, true);
    bm.set(66, true);
    bm.set(128, true);
    bm.set(191, true);

    REQUIRE(bm.select_from_low(0) == 3);
    REQUIRE(bm.select_from_low(1) == 65);
    REQUIRE(bm.select_from_low(2) == 66);
    REQUIRE(bm.select_from_low(3) == 128);
    REQUIRE(bm.select_from_low(4) == 191);

    REQUIRE(bm.select_from_high(0) == 191);
    REQUIRE(bm.select_from_high(1) == 128);
    REQUIRE(bm.select_from_high(2) == 66);
    REQUIRE(bm.select_from_high(3) == 65);
    REQUIRE(bm.select_from_high(4) == 3);

    REQUIRE(bm.find_lowest() == 3);
    REQUIRE(bm.find_highest() == 191);
}
