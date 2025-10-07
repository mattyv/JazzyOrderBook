#pragma once

#include <list>
#include <jazzy/traits.hpp>
#include <memory_resource>

namespace jazzy {
namespace detail {

// Default aggregate-only storage (no per-order tracking)
template <typename OrderType>
struct aggregate_level_storage
{
    using order_type = OrderType;
    using queue_iterator = void*; // Dummy type for template compatibility

    aggregate_level_storage() = default;

    aggregate_level_storage(const aggregate_level_storage&) = default;
    aggregate_level_storage(aggregate_level_storage&&) noexcept = default;
    aggregate_level_storage& operator=(const aggregate_level_storage&) = default;
    aggregate_level_storage& operator=(aggregate_level_storage&&) noexcept = default;
};

// FIFO maintains order queue per price level
template <typename OrderType>
struct fifo_level_storage
{
    using order_type = OrderType;
    using id_type = order_id_type<order_type>;
    using order_queue = std::pmr::list<id_type>;
    using queue_iterator = typename order_queue::iterator;

    order_queue orders;

    fifo_level_storage() = default;

    fifo_level_storage(const fifo_level_storage&) = default;
    fifo_level_storage(fifo_level_storage&&) noexcept(std::is_nothrow_move_constructible_v<order_queue>) = default;
    fifo_level_storage& operator=(const fifo_level_storage&) = default;
    fifo_level_storage& operator=(fifo_level_storage&&) noexcept(std::is_nothrow_move_assignable_v<order_queue>) =
        default;

    explicit fifo_level_storage(std::pmr::memory_resource* resource)
        : orders(resource)
    {}

    fifo_level_storage(const fifo_level_storage& other, std::pmr::memory_resource* resource)
        : orders(other.orders, resource)
    {}

    fifo_level_storage(fifo_level_storage&& other, std::pmr::memory_resource* resource)
        : orders(std::move(other.orders), resource)
    {}
};

} // namespace detail
} // namespace jazzy
