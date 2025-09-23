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

struct order_update_type_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_update_type_getter(obj);
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
using order_update_type = decltype(detail::order_update_type_getter{}(std::declval<T>()));

template <typename T>
concept order = requires(T const& order) {
    { detail::order_id_getter{}(order) } -> std::same_as<order_id_type<T>>;
    { detail::order_volume_getter{}(order) } -> std::same_as<order_volume_type<T>>;
    // Volume type must be addable
    requires requires(order_volume_type<T> v1, order_volume_type<T> v2) {
        { v1 + v2 } -> std::same_as<order_volume_type<T>>;
    };
};

constexpr detail::order_id_getter order_id_getter;
constexpr detail::order_volume_getter order_volume_getter;
constexpr detail::order_side_getter order_side_getter;

template <typename TickType, typename OrderType>
requires order<OrderType> && std::integral<TickType>
class order_book
{
    using order_type = OrderType;
    using tick_type = TickType;
    using base_type = TickType;
    using volume_type = order_volume_type<order_type>;

    struct level
    {
        volume_type volume{};
    };

    using bid_storage = std::vector<level>;
    using ask_storage = std::vector<level>;

public:
    using value_type = OrderType;

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_bid(U&& order)
    {
        std::cout << "INSERT BID - OrderId: " << order_id_getter(order) << std::endl;
        std::cout << "INSERT BID - Volume: " << order_volume_getter(order) << std::endl;
        // TODO: Add bid to appropriate price level
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(U&& order)
    {
        std::cout << "INSERT ASK - OrderId: " << order_id_getter(order) << std::endl;
        std::cout << "INSERT ASK - Volume: " << order_volume_getter(order) << std::endl;
        // TODO: Add ask to appropriate price level
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(U&& order)
    {
        std::cout << "UPDATE BID - OrderId: " << order_id_getter(order) << std::endl;
        std::cout << "UPDATE BID - Volume: " << order_volume_getter(order) << std::endl;
        // TODO: Modify existing bid
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_ask(U&& order)
    {
        std::cout << "UPDATE ASK - OrderId: " << order_id_getter(order) << std::endl;
        std::cout << "UPDATE ASK - Volume: " << order_volume_getter(order) << std::endl;
        // TODO: Modify existing ask
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_bid(U&& order)
    {
        std::cout << "REMOVE BID - OrderId: " << order_id_getter(order) << std::endl;
        // TODO: Remove bid from price level
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(U&& order)
    {
        std::cout << "REMOVE ASK - OrderId: " << order_id_getter(order) << std::endl;
        // TODO: Remove ask from price level
    }

private:
    bid_storage bids_;
    ask_storage asks_;
};

} // namespace jazzy
