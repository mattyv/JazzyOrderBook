#pragma once

#include <jazzy/traits.hpp>
#include <jazzy/detail/intrusive_fifo.hpp>
#include <optional>
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
    using queue_type = intrusive_fifo_queue<id_type>;

    queue_type queue;

    fifo_level_storage() = default;

    fifo_level_storage(const fifo_level_storage&) = default;
    fifo_level_storage(fifo_level_storage&&) noexcept(std::is_nothrow_move_constructible_v<queue_type>) = default;
    fifo_level_storage& operator=(const fifo_level_storage&) = default;
    fifo_level_storage& operator=(fifo_level_storage&&) noexcept(std::is_nothrow_move_assignable_v<queue_type>) =
        default;

    explicit fifo_level_storage(std::pmr::memory_resource* /*resource*/)
    {}

    fifo_level_storage(const fifo_level_storage& other, std::pmr::memory_resource* /*resource*/)
        : queue(other.queue)
    {}

    fifo_level_storage(fifo_level_storage&& other, std::pmr::memory_resource* /*resource*/)
        : queue(std::move(other.queue))
    {}
};

} // namespace detail
} // namespace jazzy
