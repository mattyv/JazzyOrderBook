#pragma once

namespace jazzy {
namespace detail {

// Policy: zero volume means delete the order entirely
struct zero_volume_as_delete_policy
{};

// Policy: zero volume is a valid order state (default behavior)
struct zero_volume_as_valid_policy
{};

} // namespace detail
} // namespace jazzy
