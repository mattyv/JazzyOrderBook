#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <stdexcept>
#include <utility>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

#if defined(__clang__) || defined(__GNUC__)
#define JAZZY_HAS_NO_SANITIZE_UNDEFINED 1
#define JAZZY_NO_SANITIZE_UNDEFINED __attribute__((no_sanitize("undefined")))
#else
#define JAZZY_HAS_NO_SANITIZE_UNDEFINED 0
#define JAZZY_NO_SANITIZE_UNDEFINED
#endif

#ifndef JAZZY_HAS_BUILTIN_POPCOUNT
#if defined(__clang__) || defined(__GNUC__)
#define JAZZY_HAS_BUILTIN_POPCOUNT 1
#else
#define JAZZY_HAS_BUILTIN_POPCOUNT 0
#endif
#endif

#ifndef JAZZY_HAS_BUILTIN_CTZ
#if defined(__clang__) || defined(__GNUC__)
#define JAZZY_HAS_BUILTIN_CTZ 1
#else
#define JAZZY_HAS_BUILTIN_CTZ 0
#endif
#endif

#ifndef JAZZY_HAS_BUILTIN_CLZ
#if defined(__clang__) || defined(__GNUC__)
#define JAZZY_HAS_BUILTIN_CLZ 1
#endif
#endif
#ifndef JAZZY_HAS_BUILTIN_CLZ
#define JAZZY_HAS_BUILTIN_CLZ 0
#endif

namespace jazzy::detail {

namespace level_bitmap_detail {

inline constexpr std::size_t bits_per_block = 64;

// Generate SWAR (SIMD Within A Register) bit masks by repeating a pattern
// For example: repeat_byte_pattern(0x01) = 0x0101010101010101
//              repeat_byte_pattern(0x33) = 0x3333333333333333
[[nodiscard]] constexpr std::uint64_t repeat_byte_pattern(std::uint8_t pattern) noexcept
{
    std::uint64_t result = pattern;
    result |= result << 8;
    result |= result << 16;
    result |= result << 32;
    return result;
}

// Generate alternating masks with 16-bit pattern
// For example: repeat_word_pattern(0x00ff) = 0x00ff00ff00ff00ff
[[nodiscard]] constexpr std::uint64_t repeat_word_pattern(std::uint16_t pattern) noexcept
{
    std::uint64_t result = pattern;
    result |= result << 16;
    result |= result << 32;
    return result;
}

[[nodiscard]] inline constexpr std::size_t popcount(std::uint64_t value) noexcept
{
#if JAZZY_HAS_BUILTIN_POPCOUNT
    return static_cast<std::size_t>(__builtin_popcountll(value));
#else
    // SWAR (SIMD Within A Register) parallel bit counting
    constexpr auto mask_01 = repeat_byte_pattern(0x55); // 0101 pattern
    constexpr auto mask_00_11 = repeat_byte_pattern(0x33); // 0011 pattern
    constexpr auto mask_nibble = repeat_byte_pattern(0x0f); // 00001111 pattern

    value = value - ((value >> 1) & mask_01);
    value = (value & mask_00_11) + ((value >> 2) & mask_00_11);
    value = (value + (value >> 4)) & mask_nibble;
    value = value + (value >> 8);
    value = value + (value >> 16);
    value = value + (value >> 32);
    return static_cast<std::size_t>(value & 0x7fULL);
#endif
}

#if JAZZY_HAS_NO_SANITIZE_UNDEFINED
JAZZY_NO_SANITIZE_UNDEFINED inline constexpr int lsb_index_debruijn(std::uint64_t value) noexcept
{
    static constexpr std::uint64_t debruijn = 0x03f79d71b4cb0a89ULL;
    static constexpr int table[64] = {0,  1,  48, 2,  57, 49, 28, 3,  61, 58, 50, 42, 38, 29, 17, 4,
                                      62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,
                                      63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
                                      46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9,  13, 8,  7,  6};
    const std::uint64_t isolated_bit = value & (~value + 1);
    return table[((isolated_bit)*debruijn) >> 58];
}

JAZZY_NO_SANITIZE_UNDEFINED inline constexpr unsigned int select_bit_from_msb_branchless(
    std::uint64_t value,
    unsigned int rank) noexcept
{
    constexpr unsigned int SIGN_BIT_MASK = 256; // 0x100, used to detect negative subtraction
    constexpr unsigned int BITS_PER_UINT64 = 64;

    unsigned int rank_one_indexed = rank + 1; // Algorithm uses 1-indexed rank

    // Parallel bit count for 64-bit integer (SWAR technique)
    constexpr auto mask_01 = repeat_byte_pattern(0x55); // 0101 pattern
    constexpr auto mask_00_11 = repeat_byte_pattern(0x33); // 0011 pattern
    constexpr auto mask_nibble = repeat_byte_pattern(0x0f); // 00001111 pattern
    constexpr auto mask_byte = repeat_word_pattern(0x00ff); // alternating bytes

    std::uint64_t a = value - ((value >> 1) & mask_01);
    std::uint64_t b = (a & mask_00_11) + ((a >> 2) & mask_00_11);
    std::uint64_t c = (b + (b >> 4)) & mask_nibble;
    std::uint64_t d = (c + (c >> 8)) & mask_byte;
    unsigned int bit_count = static_cast<unsigned int>((d >> 32) + (d >> 48));

    unsigned int position = BITS_PER_UINT64;

    position -= ((bit_count - rank_one_indexed) & SIGN_BIT_MASK) >> 3;
    rank_one_indexed -= (bit_count & ((bit_count - rank_one_indexed) >> 8));

    bit_count = static_cast<unsigned int>((d >> (position - 16)) & 0xff);
    position -= ((bit_count - rank_one_indexed) & SIGN_BIT_MASK) >> 4;
    rank_one_indexed -= (bit_count & ((bit_count - rank_one_indexed) >> 8));

    bit_count = static_cast<unsigned int>((c >> (position - 8)) & 0xf);
    position -= ((bit_count - rank_one_indexed) & SIGN_BIT_MASK) >> 5;
    rank_one_indexed -= (bit_count & ((bit_count - rank_one_indexed) >> 8));

    bit_count = static_cast<unsigned int>((b >> (position - 4)) & 0x7);
    position -= ((bit_count - rank_one_indexed) & SIGN_BIT_MASK) >> 6;
    rank_one_indexed -= (bit_count & ((bit_count - rank_one_indexed) >> 8));

    bit_count = static_cast<unsigned int>((a >> (position - 2)) & 0x3);
    position -= ((bit_count - rank_one_indexed) & SIGN_BIT_MASK) >> 7;
    rank_one_indexed -= (bit_count & ((bit_count - rank_one_indexed) >> 8));

    bit_count = static_cast<unsigned int>((value >> (position - 1)) & 0x1);
    position -= ((bit_count - rank_one_indexed) & SIGN_BIT_MASK) >> 8;

    position = 65 - position;
    return BITS_PER_UINT64 - position;
}
#endif

[[nodiscard]] inline constexpr int lsb_index(std::uint64_t value) noexcept
{
    assert(value != 0);
#if JAZZY_HAS_BUILTIN_CTZ
    return __builtin_ctzll(value);
#elif defined(__cpp_lib_bitops) && (__cpp_lib_bitops >= 201907L)
    return static_cast<int>(std::countr_zero(value));
#elif defined(_MSC_VER) && !defined(__clang__)
    if (std::is_constant_evaluated())
    {
        // Fallback for compile-time evaluation
        unsigned int shift = 0;
        while ((value & 1ULL) == 0ULL)
        {
            value >>= 1;
            ++shift;
        }
        return static_cast<int>(shift);
    }
    else
    {
        unsigned long index{};
        _BitScanForward64(&index, value);
        return static_cast<int>(index);
    }
#elif JAZZY_HAS_NO_SANITIZE_UNDEFINED
    return lsb_index_debruijn(value);
#else
    unsigned int shift = 0;
    while ((value & 1ULL) == 0ULL)
    {
        value >>= 1;
        ++shift;
    }
    return static_cast<int>(shift);
#endif
}

// Select the bit position (from MSB) with the given rank
// Based on: https://graphics.stanford.edu/~seander/bithacks.html
// Section: "Select the bit position (from the most-significant bit) with the given count (rank)"
// rank is 0-indexed (0 = highest bit, 1 = second highest, etc.)
[[nodiscard]] inline constexpr unsigned int select_bit_from_msb(std::uint64_t value, unsigned int rank) noexcept
{
    assert(rank < popcount(value) && "rank must be less than popcount");

#if JAZZY_HAS_BUILTIN_CLZ && JAZZY_HAS_BUILTIN_POPCOUNT
    // Use fast builtin path when available
    std::uint64_t word = value;
    for (unsigned int i = 0; i < rank; ++i)
    {
        const auto msb = 63U - static_cast<unsigned int>(__builtin_clzll(word));
        word &= ~(1ULL << msb);
    }
    return 63U - static_cast<unsigned int>(__builtin_clzll(word));
#elif defined(__cpp_lib_bitops) && (__cpp_lib_bitops >= 201907L)
    std::uint64_t word = value;
    for (unsigned int i = 0; i < rank; ++i)
    {
        const auto msb = 63U - static_cast<unsigned int>(std::countl_zero(word));
        word &= ~(1ULL << msb);
    }
    return 63U - static_cast<unsigned int>(std::countl_zero(word));
#elif JAZZY_HAS_NO_SANITIZE_UNDEFINED
    return select_bit_from_msb_branchless(value, rank);
#else
    const unsigned int total_bits = static_cast<unsigned int>(popcount(value));
    const unsigned int target_from_lsb = total_bits - rank - 1;

    std::uint64_t word = value;
    for (unsigned int i = 0; i < target_from_lsb; ++i)
    {
        word &= (word - 1); // clear lowest set bit each iteration
    }
    return static_cast<unsigned int>(lsb_index(word));
#endif
}

} // namespace level_bitmap_detail

template <
    std::size_t N,
    bool SingleBlock = ((N + level_bitmap_detail::bits_per_block - 1) / level_bitmap_detail::bits_per_block) == 1>
class level_bitmap
{
    static constexpr std::size_t bits_per_block = level_bitmap_detail::bits_per_block;
    static constexpr std::size_t block_count = (N + bits_per_block - 1) / bits_per_block;

public:
    constexpr level_bitmap() = default;
    level_bitmap(const level_bitmap&) = default;
    level_bitmap(level_bitmap&&) noexcept = default;
    level_bitmap& operator=(const level_bitmap&) = default;
    level_bitmap& operator=(level_bitmap&&) noexcept = default;

    [[nodiscard]] constexpr std::size_t size() const noexcept { return N; }

    [[nodiscard]] bool test(std::size_t index) const noexcept
    {
        assert(index < N && "level_bitmap::test index out of range");
        const auto [block, mask] = block_and_mask(index);
        return (blocks_[block] & mask) != 0;
    }

    void set(std::size_t index, bool value) noexcept
    {
        assert(index < N && "level_bitmap::set index out of range");
        const auto [block, mask] = block_and_mask(index);
        const bool currently_set = (blocks_[block] & mask) != 0;
        if (currently_set == value)
        {
            return;
        }

        if (value)
        {
            blocks_[block] |= mask;
            ++total_popcount_;
        }
        else
        {
            blocks_[block] &= ~mask;
            assert(total_popcount_ > 0);
            --total_popcount_;
        }
    }

    [[nodiscard]] bool none() const noexcept { return total_popcount_ == 0; }
    [[nodiscard]] bool any() const noexcept { return total_popcount_ != 0; }

    [[nodiscard]] std::size_t count() const noexcept { return total_popcount_; }

    [[nodiscard]] int find_lowest() const noexcept
    {
        for (std::size_t block = 0; block < block_count; ++block)
        {
            const std::uint64_t word = blocks_[block];
            if (word != 0)
            {
                return static_cast<int>(
                    block * bits_per_block + static_cast<std::size_t>(level_bitmap_detail::lsb_index(word)));
            }
        }
        return -1;
    }

    [[nodiscard]] int find_highest() const noexcept
    {
        for (std::size_t block = block_count; block > 0;)
        {
            --block;
            const std::uint64_t word = blocks_[block];
            if (word != 0)
            {
                return static_cast<int>(
                    block * bits_per_block +
                    static_cast<std::size_t>(level_bitmap_detail::select_bit_from_msb(word, 0)));
            }
        }
        return -1;
    }

    [[nodiscard]] std::size_t select_from_low(std::size_t rank) const
    {
        if (rank >= total_popcount_)
        {
            throw std::out_of_range("rank out of range");
        }

        std::size_t remaining = rank;
        for (std::size_t block = 0; block < block_count; ++block)
        {
            std::uint64_t word = blocks_[block];
            const std::size_t block_pop = level_bitmap_detail::popcount(word);
            if (remaining < block_pop)
            {
                for (std::size_t i = 0; i < remaining; ++i)
                {
                    word &= word - 1; // drop lowest set bit
                }
                assert(word != 0);
                const auto bit_offset = static_cast<std::size_t>(level_bitmap_detail::lsb_index(word));
                return block * bits_per_block + bit_offset;
            }
            remaining -= block_pop;
        }

        throw std::logic_error("Failed to locate bit in level_bitmap");
    }

    [[nodiscard]] std::size_t select_from_high(std::size_t rank) const
    {
        if (rank >= total_popcount_)
        {
            throw std::out_of_range("rank out of range");
        }

        std::size_t remaining = rank;
        for (std::size_t block = block_count; block > 0;)
        {
            --block;
            std::uint64_t word = blocks_[block];
            const std::size_t block_pop = level_bitmap_detail::popcount(word);
            if (remaining < block_pop)
            {
                const auto bit_offset =
                    level_bitmap_detail::select_bit_from_msb(word, static_cast<unsigned int>(remaining));
                return block * bits_per_block + bit_offset;
            }
            remaining -= block_pop;
        }

        throw std::logic_error("Failed to locate bit in level_bitmap");
    }

private:
    [[nodiscard]] static constexpr std::pair<std::size_t, uint64_t> block_and_mask(std::size_t index) noexcept
    {
        const std::size_t block = index / bits_per_block;
        const std::size_t offset = index % bits_per_block;
        return {block, 1ull << offset};
    }

    std::array<uint64_t, block_count> blocks_{};
    std::size_t total_popcount_ = 0;
};

template <std::size_t N>
class level_bitmap<N, true>
{
    static_assert(N > 0);
    static_assert(N <= level_bitmap_detail::bits_per_block);

public:
    constexpr level_bitmap() = default;
    level_bitmap(const level_bitmap&) = default;
    level_bitmap(level_bitmap&&) noexcept = default;
    level_bitmap& operator=(const level_bitmap&) = default;
    level_bitmap& operator=(level_bitmap&&) noexcept = default;

    [[nodiscard]] constexpr std::size_t size() const noexcept { return N; }

    [[nodiscard]] bool test(std::size_t index) const noexcept
    {
        assert(index < N && "level_bitmap::test index out of range");
        const auto mask = 1ull << index;
        return (bits_ & mask) != 0;
    }

    void set(std::size_t index, bool value) noexcept
    {
        assert(index < N && "level_bitmap::set index out of range");
        const auto mask = 1ull << index;
        const bool currently_set = (bits_ & mask) != 0;
        if (currently_set == value)
        {
            return;
        }

        if (value)
        {
            bits_ |= mask;
            ++total_popcount_;
        }
        else
        {
            bits_ &= ~mask;
            assert(total_popcount_ > 0);
            --total_popcount_;
        }
    }

    [[nodiscard]] bool none() const noexcept { return bits_ == 0; }
    [[nodiscard]] bool any() const noexcept { return bits_ != 0; }

    [[nodiscard]] std::size_t count() const noexcept { return total_popcount_; }

    [[nodiscard]] int find_lowest() const noexcept
    {
        if (bits_ == 0)
        {
            return -1;
        }
        return level_bitmap_detail::lsb_index(bits_);
    }

    [[nodiscard]] int find_highest() const noexcept
    {
        if (bits_ == 0)
        {
            return -1;
        }
        return static_cast<int>(level_bitmap_detail::select_bit_from_msb(bits_, 0));
    }

    [[nodiscard]] std::size_t select_from_low(std::size_t rank) const
    {
        if (rank >= total_popcount_)
        {
            throw std::out_of_range("rank out of range");
        }

        std::uint64_t word = bits_;
        for (std::size_t i = 0; i < rank; ++i)
        {
            word &= word - 1; // drop lowest set bit
        }
        assert(word != 0);
        return static_cast<std::size_t>(level_bitmap_detail::lsb_index(word));
    }

    [[nodiscard]] std::size_t select_from_high(std::size_t rank) const
    {
        if (rank >= total_popcount_)
        {
            throw std::out_of_range("rank out of range");
        }

        return static_cast<std::size_t>(
            level_bitmap_detail::select_bit_from_msb(bits_, static_cast<unsigned int>(rank)));
    }

private:
    std::uint64_t bits_ = 0;
    std::size_t total_popcount_ = 0;
};

} // namespace jazzy::detail
