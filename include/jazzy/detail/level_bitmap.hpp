#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

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

[[nodiscard]] inline constexpr std::size_t popcount(std::uint64_t value) noexcept
{
#if JAZZY_HAS_BUILTIN_POPCOUNT
    return static_cast<std::size_t>(__builtin_popcountll(value));
#else
    value = value - ((value >> 1) & 0x5555555555555555ULL);
    value = (value & 0x3333333333333333ULL) + ((value >> 2) & 0x3333333333333333ULL);
    value = (value + (value >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    value = value + (value >> 8);
    value = value + (value >> 16);
    value = value + (value >> 32);
    return static_cast<std::size_t>(value & 0x7fULL);
#endif
}

[[nodiscard]] inline constexpr int lsb_index(std::uint64_t value) noexcept
{
    assert(value != 0);
#if JAZZY_HAS_BUILTIN_CTZ
    return __builtin_ctzll(value);
#else
    static constexpr std::uint64_t debruijn = 0x03f79d71b4cb0a89ULL;
    static constexpr int table[64] = {0,  1,  48, 2,  57, 49, 28, 3,  61, 58, 50, 42, 38, 29, 17, 4,
                                      62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,
                                      63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
                                      46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9,  13, 8,  7,  6};
    return table[((value & -value) * debruijn) >> 58];
#endif
}

[[nodiscard]] inline constexpr int msb_index(std::uint64_t value) noexcept
{
    assert(value != 0);
#if JAZZY_HAS_BUILTIN_CLZ
    return 63 - __builtin_clzll(value);
#else
    int index = 0;
    if (value >= (1ULL << 32))
    {
        value >>= 32;
        index += 32;
    }
    if (value >= (1ULL << 16))
    {
        value >>= 16;
        index += 16;
    }
    if (value >= (1ULL << 8))
    {
        value >>= 8;
        index += 8;
    }
    if (value >= (1ULL << 4))
    {
        value >>= 4;
        index += 4;
    }
    if (value >= (1ULL << 2))
    {
        value >>= 2;
        index += 2;
    }
    if (value >= (1ULL << 1))
    {
        index += 1;
    }
    return index;
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
        for (std::size_t block = block_count; block-- > 0;)
        {
            const std::uint64_t word = blocks_[block];
            if (word != 0)
            {
                return static_cast<int>(
                    block * bits_per_block + static_cast<std::size_t>(level_bitmap_detail::msb_index(word)));
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
                while (remaining--)
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
        for (std::size_t block = block_count; block-- > 0;)
        {
            std::uint64_t word = blocks_[block];
            const std::size_t block_pop = level_bitmap_detail::popcount(word);
            if (remaining < block_pop)
            {
                while (remaining--)
                {
                    const auto msb = level_bitmap_detail::msb_index(word);
                    word &= ~(1ull << msb);
                }
                assert(word != 0);
                const auto bit_offset = static_cast<std::size_t>(level_bitmap_detail::msb_index(word));
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
        return level_bitmap_detail::msb_index(bits_);
    }

    [[nodiscard]] std::size_t select_from_low(std::size_t rank) const
    {
        if (rank >= total_popcount_)
        {
            throw std::out_of_range("rank out of range");
        }

        std::uint64_t word = bits_;
        std::size_t remaining = rank;
        while (remaining--)
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

        std::uint64_t word = bits_;
        std::size_t remaining = rank;
        while (remaining--)
        {
            assert(word != 0);
            const auto msb = static_cast<std::size_t>(level_bitmap_detail::msb_index(word));
            word &= ~(1ull << msb);
        }
        assert(word != 0);
        return static_cast<std::size_t>(level_bitmap_detail::msb_index(word));
    }

private:
    std::uint64_t bits_ = 0;
    std::size_t total_popcount_ = 0;
};

} // namespace jazzy::detail
