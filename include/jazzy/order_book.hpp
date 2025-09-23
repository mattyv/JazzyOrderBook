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

template <typename TBaseType, typename TickType, typename OrderType>
requires order<OrderType> && std::integral<TickType>
class order_book
{
    using order_type = OrderType;
    using tick_type = TickType;
    using base_type = TickType;
    using volume_type = order_volume_type<order_type>;

    struct level {
        volume_type volume{}; 
    };

    using bid_storage = std::vector<level>;
    using ask_storage = std::vector<level>;

public:
    using value_type = OrderType;

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert(U&& order)
    {
        std::cout << "OrderId: " << order_id_getter(order) << std::endl;
        std::cout << "Order Volume: " << order_volume_getter(order) << std::endl;
    }

private:
    bid_storage bids_;
    ask_storage asks_;
};

} // namespace jazzy
