#pragma once
#include <concepts>
#include <cstddef>
#include <memory>

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

template <typename T>
concept compatible_allocator = requires(T alloc) {
    typename std::allocator_traits<T>::value_type;
    { alloc.allocate(1) } -> std::same_as<typename std::allocator_traits<T>::value_type*>;
    { alloc.deallocate(std::declval<typename std::allocator_traits<T>::value_type*>(), 1) } -> std::same_as<void>;
    requires std::is_copy_constructible_v<T>;
    requires std::is_move_constructible_v<T>;
    requires std::is_copy_assignable_v<T>;
    requires std::is_move_assignable_v<T>;
    requires requires { typename std::allocator_traits<T>::template rebind_alloc<int>; };
};

constexpr detail::order_id_getter order_id_getter;
constexpr detail::order_volume_getter order_volume_getter;
constexpr detail::order_tick_getter order_tick_getter;
constexpr detail::order_volume_setter order_volume_setter;
constexpr detail::order_tick_setter order_tick_setter;

} // namespace jazzy
