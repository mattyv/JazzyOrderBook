#pragma once

#include "traits.hpp"
#include <limits>

namespace jazzy {

template <typename TickType>
requires tick<TickType>
class tick_type_strong
{
public:
    using rep = TickType;

    explicit constexpr tick_type_strong(TickType v) noexcept
        : v_(v)
        , has_value_(true)
    {}
    explicit constexpr tick_type_strong(std::size_t v) noexcept
        : v_(static_cast<TickType>(v))
        , has_value_(true)
    {}
    constexpr tick_type_strong() noexcept(std::is_nothrow_default_constructible_v<TickType>) = default;

    // Create invalid/sentinel value
    static constexpr tick_type_strong no_value() noexcept { return tick_type_strong{}; }

    explicit constexpr operator TickType() const noexcept { return v_; }
    constexpr TickType value() const noexcept { return v_; }
    constexpr bool has_value() const noexcept { return has_value_; }
    constexpr bool is_valid() const noexcept { return has_value_; }

    // Custom comparison operators to handle sentinel values properly
    constexpr bool operator<(const tick_type_strong& rhs) const noexcept
    {
        if (!has_value_)
            return false; // invalid is never less than anything
        if (!rhs.has_value_)
            return true; // valid is always less than invalid
        return v_ < rhs.v_; // both valid, compare values
    }

    constexpr bool operator>(const tick_type_strong& rhs) const noexcept { return rhs < *this; }

    constexpr bool operator<=(const tick_type_strong& rhs) const noexcept { return !(*this > rhs); }

    constexpr bool operator>=(const tick_type_strong& rhs) const noexcept { return !(*this < rhs); }

    constexpr bool operator==(const tick_type_strong& rhs) const noexcept
    {
        if (has_value_ != rhs.has_value_)
            return false;
        return !has_value_ || v_ == rhs.v_; // both invalid OR both valid and equal
    }

    constexpr bool operator!=(const tick_type_strong& rhs) const noexcept { return !(*this == rhs); }

    // explicit converstion to size_type
    explicit constexpr operator std::size_t() const noexcept { return static_cast<std::size_t>(v_); }
private:
    TickType v_{};
    bool has_value_{false};
};

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
    using tick_type_strong_t = tick_type_strong<TickType>;
    static constexpr tick_type_strong_t daily_high_v = tick_type_strong_t{daily_high};
    static constexpr tick_type_strong_t daily_low_v = tick_type_strong_t{daily_low};
    static constexpr tick_type_strong_t daily_close_v = tick_type_strong_t{daily_close};
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
