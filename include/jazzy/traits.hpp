#pragma once
#include <concepts>
#include <cstddef>

namespace jazzy {
namespace detail {

struct order_id_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_id_getter(obj);
    }
};

struct order_volume_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_volume_getter(obj);
    }
};

struct order_volume_setter
{
    template <typename T, typename U>
    constexpr void operator()(T& obj, U const& value) const
    {
        jazzy_order_volume_setter(obj, value);
    }
};

struct order_tick_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_tick_getter(obj);
    }
};

struct order_tick_setter
{
    template <typename T, typename U>
    constexpr void operator()(T& obj, U const& value) const
    {
        jazzy_order_tick_setter(obj, value);
    }
};

} // namespace detail

template <typename T>
using order_id_type = decltype(detail::order_id_getter{}(std::declval<T>()));

template <typename T>
using order_volume_type = decltype(detail::order_volume_getter{}(std::declval<T>()));

template <typename T>
using order_tick_type = decltype(detail::order_tick_getter{}(std::declval<T>()));

template <typename T>
concept tick = std::integral<T> && std::convertible_to<T, size_t> && std::convertible_to<T, unsigned long>;

template <typename T>
requires tick<T>
using tick_type = T;

template <typename T>
concept order = requires(T const& order) {
    { detail::order_id_getter{}(order) } -> std::same_as<order_id_type<T>>;
    { detail::order_volume_getter{}(order) } -> std::same_as<order_volume_type<T>>;
    { detail::order_tick_getter{}(order) } -> std::same_as<order_tick_type<T>>;
    { detail::order_volume_setter{}(std::declval<T&>(), std::declval<order_volume_type<T>>()) } -> std::same_as<void>;
    { detail::order_tick_setter{}(std::declval<T&>(), std::declval<order_tick_type<T>>()) } -> std::same_as<void>;
    requires requires(order_volume_type<T> v1, order_volume_type<T> v2) {
        { v1 + v2 } -> std::same_as<order_volume_type<T>>;
    };
    requires tick<order_tick_type<T>>;
};

constexpr detail::order_id_getter order_id_getter;
constexpr detail::order_volume_getter order_volume_getter;
constexpr detail::order_tick_getter order_tick_getter;
constexpr detail::order_volume_setter order_volume_setter;
constexpr detail::order_tick_setter order_tick_setter;

// Forward declare storage policies
namespace detail {
template <typename OrderType>
struct aggregate_level_storage;

template <typename OrderType>
struct fifo_level_storage;
} // namespace detail

// Type trait to detect if a storage policy supports FIFO
template <typename T>
struct is_fifo_storage : std::false_type
{};

template <typename OrderType>
struct is_fifo_storage<detail::fifo_level_storage<OrderType>> : std::true_type
{};

template <typename T>
inline constexpr bool is_fifo_storage_v = is_fifo_storage<T>::value;

} // namespace jazzy
