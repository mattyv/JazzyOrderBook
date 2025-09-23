#pragma once

#include "traits.hpp"

namespace jazzy {

template <
    typename TickType,
    TickType daily_high,
    TickType daily_low,
    TickType daily_close,
    unsigned expected_range_basis_points>
requires tick<TickType>
struct market_statistics
{
    using tick_type = TickType;
    static constexpr tick_type daily_high_v = daily_high;
    static constexpr tick_type daily_low_v = daily_low;
    static constexpr tick_type daily_close_v = daily_close;
    static constexpr double expected_range_v = expected_range_basis_points / 10000.0;
};

template <typename T>
concept market_stats = requires {
    typename T::tick_type;
    T::daily_high_v;
    T::daily_low_v;
    T::daily_close_v;
    T::expected_range_v;
    requires tick<typename T::tick_type>;
};

} // namespace jazzy
