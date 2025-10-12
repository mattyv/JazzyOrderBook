#pragma once

namespace jazzy {
namespace detail {

// Policy: assert if order price is out of bounds (current behavior)
struct bounds_check_assert_policy
{};

// Policy: silently discard orders with prices outside bounds
struct bounds_check_discard_policy
{};

} // namespace detail
} // namespace jazzy
