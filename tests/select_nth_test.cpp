#include "catch.hpp"
#include <array>
#include <bit>
#include <jazzy/detail/select_nth.hpp>
#include <jazzy/order_book.hpp>
#include <vector>

namespace jazzy::tests {

// Compile-time binary literal conversion
template <size_t N>
constexpr uint64_t binary_literal(const char (&str)[N])
{
    static_assert(N <= 65, "Binary string too long for uint64_t");
    uint64_t result = 0;
    for (size_t i = 0; i < N - 1; ++i)
    {
        if (str[i] == '1')
        {
            result = (result << 1) | 1;
        }
        else if (str[i] == '0')
        {
            result = result << 1;
        }
    }
    return result;
}

// Named constants for bit positions
constexpr int BIT_0 = 0;
constexpr int BIT_1 = 1;
constexpr int BIT_2 = 2;
constexpr int BIT_3 = 3;
constexpr int BIT_7 = 7;
constexpr int BIT_62 = 62;
constexpr int BIT_63 = 63;

// Named constants for common patterns
constexpr uint64_t SINGLE_BIT_0 = binary_literal("1");
constexpr uint64_t SINGLE_BIT_1 = binary_literal("10");
constexpr uint64_t SINGLE_BIT_2 = binary_literal("100");
constexpr uint64_t SINGLE_BIT_3 = binary_literal("1000");

constexpr uint64_t TWO_ADJACENT_BITS = binary_literal("11");
constexpr uint64_t TWO_SPACED_BITS = binary_literal("101");
constexpr uint64_t ALTERNATING_BITS = binary_literal("1010");
constexpr uint64_t ALL_BYTE_BITS = binary_literal("11111111");

SCENARIO("Finding the nth set bit in simple patterns", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("A mask with only bit 0 set")
    {
        constexpr auto mask = SINGLE_BIT_0;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 0") { REQUIRE(result == BIT_0); }
        }
    }

    GIVEN("A mask with only bit 1 set")
    {
        constexpr auto mask = SINGLE_BIT_1;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 1") { REQUIRE(result == BIT_1); }
        }
    }

    GIVEN("A mask with only bit 2 set")
    {
        constexpr auto mask = SINGLE_BIT_2;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 2") { REQUIRE(result == BIT_2); }
        }
    }

    GIVEN("A mask with only bit 3 set")
    {
        constexpr auto mask = SINGLE_BIT_3;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 3") { REQUIRE(result == BIT_3); }
        }
    }
}

SCENARIO("Finding the nth set bit in multiple bit patterns", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("A mask with two adjacent bits set (11)")
    {
        constexpr auto mask = TWO_ADJACENT_BITS;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 0") { REQUIRE(result == BIT_0); }
        }

        WHEN("Finding the 1st set bit")
        {
            auto result = select_nth_set_bit(mask, 1);

            THEN("It should be at position 1") { REQUIRE(result == BIT_1); }
        }
    }

    GIVEN("A mask with two spaced bits set (101)")
    {
        constexpr auto mask = TWO_SPACED_BITS;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 0") { REQUIRE(result == BIT_0); }
        }

        WHEN("Finding the 1st set bit")
        {
            auto result = select_nth_set_bit(mask, 1);

            THEN("It should be at position 2") { REQUIRE(result == BIT_2); }
        }
    }

    GIVEN("A mask with alternating bits (1010)")
    {
        constexpr auto mask = ALTERNATING_BITS;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 1") { REQUIRE(result == BIT_1); }
        }

        WHEN("Finding the 1st set bit")
        {
            auto result = select_nth_set_bit(mask, 1);

            THEN("It should be at position 3") { REQUIRE(result == BIT_3); }
        }
    }
}

SCENARIO("Finding bits in a full byte pattern", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("A mask with all 8 bits set in a byte")
    {
        constexpr auto mask = ALL_BYTE_BITS;

        WHEN("Finding each set bit sequentially")
        {
            THEN("Each nth bit should be at position n")
            {
                for (int n = 0; n < 8; ++n)
                {
                    REQUIRE(select_nth_set_bit(mask, n) == n);
                }
            }
        }
    }
}

// Additional constants for edge cases
constexpr uint64_t ALL_BITS_SET = ~0ULL;
constexpr uint64_t NO_BITS_SET = 0ULL;
constexpr uint64_t HIGHEST_BIT = 1ULL << 63;
constexpr uint64_t HIGHEST_TWO_BITS =
    binary_literal("1100000000000000000000000000000000000000000000000000000000000000");

// Pattern constants for sparse testing
constexpr uint64_t EVERY_OTHER_BIT = binary_literal("1010101010101010101010101010101010101010101010101010101010101010");
constexpr uint64_t EVERY_FOURTH_BIT =
    binary_literal("0001000100010001000100010001000100010001000100010001000100010001");
constexpr uint64_t CROSS_BYTE_PATTERN =
    binary_literal("0000000100000001000000010000000100000001000000010000000100000001");

SCENARIO("Finding bits in edge case patterns", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("A mask with all 64 bits set")
    {
        constexpr auto mask = ALL_BITS_SET;

        WHEN("Finding each bit sequentially")
        {
            THEN("Each nth bit should be at position n")
            {
                for (int n = 0; n < 64; ++n)
                {
                    REQUIRE(select_nth_set_bit(mask, n) == n);
                }
            }
        }
    }

    GIVEN("A mask with only the highest bit set")
    {
        constexpr auto mask = HIGHEST_BIT;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 63") { REQUIRE(result == BIT_63); }
        }
    }

    GIVEN("A mask with the two highest bits set")
    {
        constexpr auto mask = HIGHEST_TWO_BITS;

        WHEN("Finding the 0th set bit")
        {
            auto result = select_nth_set_bit(mask, 0);

            THEN("It should be at position 62") { REQUIRE(result == BIT_62); }
        }

        WHEN("Finding the 1st set bit")
        {
            auto result = select_nth_set_bit(mask, 1);

            THEN("It should be at position 63") { REQUIRE(result == BIT_63); }
        }
    }
}

SCENARIO("Finding bits in sparse patterns", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("A mask with every other bit set")
    {
        constexpr auto mask = EVERY_OTHER_BIT;
        constexpr int expected_set_bits = 32;

        WHEN("Finding each set bit")
        {
            THEN("Each nth bit should be at position 2n+1")
            {
                for (int n = 0; n < expected_set_bits; ++n)
                {
                    REQUIRE(select_nth_set_bit(mask, n) == n * 2 + 1);
                }
            }
        }
    }

    GIVEN("A mask with every fourth bit set")
    {
        constexpr auto mask = EVERY_FOURTH_BIT;
        constexpr int expected_set_bits = 16;

        WHEN("Finding each set bit")
        {
            THEN("Each nth bit should be at position 4n")
            {
                for (int n = 0; n < expected_set_bits; ++n)
                {
                    REQUIRE(select_nth_set_bit(mask, n) == n * 4);
                }
            }
        }
    }

    GIVEN("A pattern crossing byte boundaries")
    {
        constexpr auto mask = CROSS_BYTE_PATTERN;
        constexpr int expected_set_bits = 8;
        constexpr int bits_per_byte = 8;

        WHEN("Finding each set bit")
        {
            THEN("Each nth bit should be at position 8n")
            {
                for (int n = 0; n < expected_set_bits; ++n)
                {
                    REQUIRE(select_nth_set_bit(mask, n) == n * bits_per_byte);
                }
            }
        }
    }
}

SCENARIO("Testing individual bit positions", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("Each possible single bit position")
    {
        WHEN("Finding the only set bit")
        {
            THEN("The result should match the bit position")
            {
                for (int pos = 0; pos < 64; ++pos)
                {
                    uint64_t mask = 1ULL << pos;
                    REQUIRE(select_nth_set_bit(mask, 0) == pos);
                }
            }
        }
    }
}

SCENARIO("Error handling for invalid inputs", "[select_nth]")
{
    using select64::select_nth_set_bit;

    GIVEN("An empty mask with no bits set")
    {
        constexpr auto mask = NO_BITS_SET;

        WHEN("Attempting to find the 0th set bit")
        {
            THEN("It should throw out_of_range") { REQUIRE_THROWS_AS(select_nth_set_bit(mask, 0), std::out_of_range); }
        }

        WHEN("Attempting to find the 1st set bit")
        {
            THEN("It should throw out_of_range") { REQUIRE_THROWS_AS(select_nth_set_bit(mask, 1), std::out_of_range); }
        }
    }

    GIVEN("A mask with only one bit set")
    {
        constexpr auto mask = SINGLE_BIT_0;

        WHEN("Attempting to find the 1st set bit (index out of range)")
        {
            THEN("It should throw out_of_range") { REQUIRE_THROWS_AS(select_nth_set_bit(mask, 1), std::out_of_range); }
        }
    }

    GIVEN("A mask with two bits set")
    {
        constexpr auto mask = TWO_ADJACENT_BITS;

        WHEN("Attempting to find the 2nd set bit (index out of range)")
        {
            THEN("It should throw out_of_range") { REQUIRE_THROWS_AS(select_nth_set_bit(mask, 2), std::out_of_range); }
        }
    }

    GIVEN("A mask with all bits set")
    {
        constexpr auto mask = ALL_BITS_SET;

        WHEN("Attempting to find the 64th set bit (index out of range)")
        {
            THEN("It should throw out_of_range") { REQUIRE_THROWS_AS(select_nth_set_bit(mask, 64), std::out_of_range); }
        }
    }
}

// Test patterns for consistency checking
constexpr uint64_t PATTERN_SINGLE = SINGLE_BIT_0;
constexpr uint64_t PATTERN_ADJACENT = TWO_ADJACENT_BITS;
constexpr uint64_t PATTERN_SPACED = TWO_SPACED_BITS;
constexpr uint64_t PATTERN_BYTE = ALL_BYTE_BITS;
constexpr uint64_t PATTERN_SPARSE = CROSS_BYTE_PATTERN;
constexpr uint64_t PATTERN_HIGH_LOW =
    binary_literal("1000000000000000000000000000000000000000000000000000000000000001");

// Complex test patterns using readable binary literals
constexpr uint64_t PATTERN_COMPLEX_1 =
    binary_literal("0001001000110100010101100111100010001001101010111100110111101111");
constexpr uint64_t PATTERN_COMPLEX_2 =
    binary_literal("1111011101001011010000010110101100101001011000001000000100010000");
constexpr uint64_t PATTERN_ALTERNATING_BYTES =
    binary_literal("1010101010101010101010101010101010101010101010101010101010101010");
constexpr uint64_t PATTERN_CHECKERBOARD =
    binary_literal("0101010101010101010101010101010101010101010101010101010101010101");
constexpr uint64_t PATTERN_THIRDS = binary_literal("0011001100110011001100110011001100110011001100110011001100110011");
constexpr uint64_t PATTERN_NIBBLES = binary_literal("0000111100001111000011110000111100001111000011110000111100001111");

SCENARIO("Consistency between portable and BMI2 implementations", "[select_nth]")
{
    using select64::select_nth_set_bit;
    using select64::select_nth_set_bit_portable;

    constexpr std::array<uint64_t, 7> test_patterns = {
        PATTERN_SINGLE, PATTERN_ADJACENT, PATTERN_SPACED, PATTERN_BYTE, PATTERN_SPARSE, PATTERN_HIGH_LOW, ALL_BITS_SET};

    GIVEN("Various bit patterns")
    {
        WHEN("Comparing portable implementation with main API")
        {
            THEN("Both implementations should produce identical results")
            {
                for (auto mask : test_patterns)
                {
                    int popcount = std::popcount(mask);
                    for (int n = 0; n < popcount; ++n)
                    {
                        int portable_result = select_nth_set_bit_portable(mask, n);
                        int api_result = select_nth_set_bit(mask, n);
                        REQUIRE(portable_result == api_result);
                    }
                }
            }
        }
    }
}

SCENARIO("Verification against manual bit scanning", "[select_nth]")
{
    using select64::select_nth_set_bit;

    constexpr std::array<uint64_t, 6> complex_patterns = {
        PATTERN_COMPLEX_1,
        PATTERN_COMPLEX_2,
        PATTERN_ALTERNATING_BYTES,
        PATTERN_CHECKERBOARD,
        PATTERN_THIRDS,
        PATTERN_NIBBLES};

    GIVEN("Complex bit patterns")
    {
        WHEN("Finding set bits using select_nth_set_bit")
        {
            THEN("Results should match manual bit-by-bit scanning")
            {
                for (auto mask : complex_patterns)
                {
                    std::vector<int> expected_positions;

                    // Find all set bit positions manually
                    for (int pos = 0; pos < 64; ++pos)
                    {
                        if (mask & (1ULL << pos))
                        {
                            expected_positions.push_back(pos);
                        }
                    }

                    // Verify select_nth_set_bit matches our manual scan
                    for (size_t n = 0; n < expected_positions.size(); ++n)
                    {
                        REQUIRE(select_nth_set_bit(mask, n) == expected_positions[n]);
                    }
                }
            }
        }
    }
}

} // namespace jazzy::tests
