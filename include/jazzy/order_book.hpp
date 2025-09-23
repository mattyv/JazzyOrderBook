#pragma once

#include <concepts>
#include <iostream>
#include <vector>

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

struct order_side_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_side_getter(obj);
    }
};

} // namespace detail

template <typename T>
using order_id_type = decltype(detail::order_id_getter{}(std::declval<T>()));

template <typename T>
using order_volume_type = decltype(detail::order_volume_getter{}(std::declval<T>()));

template <typename T>
using order_side_type = decltype(detail::order_side_getter{}(std::declval<T>()));

template <typename T>
concept order = requires(T const& order) {
    { detail::order_id_getter{}(order) } -> std::same_as<order_id_type<T>>;
    { detail::order_volume_getter{}(order) } -> std::same_as<order_volume_type<T>>;
};

constexpr detail::order_id_getter order_id_getter;
constexpr detail::order_volume_getter order_volume_getter;
constexpr detail::order_side_getter order_side_getter;

template <typename T>
requires order<T>
class order_book
{
public:
    using value_type = T;
    using order_type = T;

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert(U&& order)
    {
        std::cout << "OrderId: " << order_id_getter(order) << std::endl;
        std::cout << "Order Volume: " << order_volume_getter(order) << std::endl;
    }
};

} // namespace jazzy
