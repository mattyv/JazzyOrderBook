#include <catch.hpp>
#include <jazzy/types.hpp>

using tick_type_strong_int = jazzy::tick_type_strong<int>;

SCENARIO("tick_type_strong handles sentinel values correctly", "[tick_type_strong]")
{
    GIVEN("Default constructed tick_type_strong")
    {
        tick_type_strong_int invalid_tick{};

        THEN("It should be invalid")
        {
            REQUIRE_FALSE(invalid_tick.has_value());
            REQUIRE_FALSE(invalid_tick.is_valid());
        }

        THEN("no_value() should create equivalent invalid tick")
        {
            auto no_val = tick_type_strong_int::no_value();
            REQUIRE_FALSE(no_val.has_value());
            REQUIRE(invalid_tick == no_val);
        }
    }

    GIVEN("Valid tick_type_strong")
    {
        tick_type_strong_int valid_tick{100};

        THEN("It should be valid")
        {
            REQUIRE(valid_tick.has_value());
            REQUIRE(valid_tick.is_valid());
            REQUIRE(valid_tick.value() == 100);
        }
    }
}

SCENARIO("tick_type_strong comparison operators handle mixed valid/invalid correctly", "[tick_type_strong]")
{
    GIVEN("Valid and invalid ticks")
    {
        tick_type_strong_int valid1{100};
        tick_type_strong_int valid2{200};
        tick_type_strong_int invalid = tick_type_strong_int::no_value();

        THEN("Valid vs valid comparisons work normally")
        {
            REQUIRE(valid1 < valid2);
            REQUIRE(valid2 > valid1);
            REQUIRE_FALSE(valid1 == valid2);
            REQUIRE(valid1 != valid2);
        }

        THEN("Invalid is never less than anything")
        {
            REQUIRE_FALSE(invalid < valid1);
            REQUIRE_FALSE(invalid < valid2);
            REQUIRE_FALSE(invalid < invalid);
        }

        THEN("Valid is always less than invalid")
        {
            REQUIRE(valid1 < invalid);
            REQUIRE(valid2 < invalid);
        }

        THEN("Invalid equals only invalid")
        {
            REQUIRE_FALSE(invalid == valid1);
            REQUIRE_FALSE(valid1 == invalid);
            REQUIRE(invalid == invalid);
        }
    }
}

SCENARIO("tick_type_strong conversion operators work correctly", "[tick_type_strong]")
{
    GIVEN("A valid tick_type_strong")
    {
        tick_type_strong_int tick{42};

        THEN("value() returns the correct value") { REQUIRE(tick.value() == 42); }

        THEN("Explicit cast to underlying type works") { REQUIRE(static_cast<int>(tick) == 42); }

        THEN("Explicit cast to size_t works") { REQUIRE(static_cast<std::size_t>(tick) == 42); }
    }
}

SCENARIO("tick_type_strong can be constructed from size_t", "[tick_type_strong]")
{
    GIVEN("A size_t value")
    {
        std::size_t val = 123;

        WHEN("Constructing tick_type_strong from size_t")
        {
            tick_type_strong_int tick{val};

            THEN("It should be valid and hold the correct value")
            {
                REQUIRE(tick.has_value());
                REQUIRE(tick.value() == 123);
            }
        }
    }
}