#pragma once

#include <deque>
#include <jazzy/traits.hpp>
#include <memory>

namespace jazzy {
namespace detail {

// Default aggregate-only storage (no per-order tracking)
template <typename OrderType, typename Allocator>
struct aggregate_level_storage
{
    using order_type = OrderType;
    using allocator_type = Allocator;
    using queue_iterator = void*; // Dummy type for template compatibility

    aggregate_level_storage() = default;

    aggregate_level_storage(const aggregate_level_storage&) = default;
    aggregate_level_storage(aggregate_level_storage&&) noexcept = default;
    aggregate_level_storage& operator=(const aggregate_level_storage&) = default;
    aggregate_level_storage& operator=(aggregate_level_storage&&) noexcept = default;

    template <typename Alloc>
    explicit aggregate_level_storage(std::allocator_arg_t, const Alloc&)
    {}

    template <typename Alloc>
    aggregate_level_storage(std::allocator_arg_t, const Alloc&, const aggregate_level_storage&)
    {}

    template <typename Alloc>
    aggregate_level_storage(std::allocator_arg_t, const Alloc&, aggregate_level_storage&&)
    {}
};

// FIFO maintains order queue per price level
template <typename OrderType, typename Allocator>
struct fifo_level_storage
{
    using order_type = OrderType;
    using allocator_type = Allocator;
    using id_type = order_id_type<order_type>;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using id_allocator_type = typename allocator_traits::template rebind_alloc<id_type>;
    using order_queue = std::deque<id_type, id_allocator_type>;
    using queue_iterator = typename order_queue::iterator;

    order_queue orders;

    fifo_level_storage() = default;

    fifo_level_storage(const fifo_level_storage&) = default;
    fifo_level_storage(fifo_level_storage&&) noexcept(std::is_nothrow_move_constructible_v<order_queue>) = default;
    fifo_level_storage& operator=(const fifo_level_storage&) = default;
    fifo_level_storage& operator=(fifo_level_storage&&) noexcept(std::is_nothrow_move_assignable_v<order_queue>) =
        default;

    template <typename Alloc>
    explicit fifo_level_storage(std::allocator_arg_t, const Alloc& alloc)
        : orders(id_allocator_type(alloc))
    {}

    template <typename Alloc>
    fifo_level_storage(std::allocator_arg_t, const Alloc& alloc, const fifo_level_storage& other)
        : orders(other.orders, id_allocator_type(alloc))
    {}

    template <typename Alloc>
    fifo_level_storage(std::allocator_arg_t, const Alloc& alloc, fifo_level_storage&& other)
        : orders(std::move(other.orders), id_allocator_type(alloc))
    {}
};

} // namespace detail
} // namespace jazzy
