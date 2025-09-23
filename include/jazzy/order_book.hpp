#pragma once

#include <concepts>
#include <iostream>
#include <unordered_map>
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

struct order_volume_setter
{
    template <typename T, typename U>
    constexpr void operator()(T& obj, U const& value) const
    {
        jazzy_order_volume_setter(obj, value);
    }
};

} // namespace detail

template <typename T>
using order_id_type = decltype(detail::order_id_getter{}(std::declval<T>()));

template <typename T>
using order_volume_type = decltype(detail::order_volume_getter{}(std::declval<T>()));

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
constexpr detail::order_volume_setter order_volume_setter;

template <typename TickType, typename OrderType>
requires order<OrderType> && std::integral<TickType>
class order_book
{
    using order_type = OrderType;
    using tick_type = TickType;
    using base_type = TickType;
    using id_type = order_id_type<order_type>;
    using volume_type = order_volume_type<order_type>;

    struct level
    {
        volume_type volume{};
    };

    using bid_storage = std::vector<level>;
    using ask_storage = std::vector<level>;
    using order_storage = std::unordered_map<id_type, order_type>;

public:
    using value_type = OrderType;
    using size_type = size_t;

    order_book(tick_type base, size_type size)
        : base_{std::move(base)}
        , size_{size}
    {
        bids_.resize(size_);
        asks_.resize(size_);
        orders_.reserve(size_);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_bid(tick_type tick_value, U&& order)
    {
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        if (!succ)
            return;

        auto offset = (tick_value - base_);

        bids_[start_ + offset].volume += order_volume_getter(order);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        if (!succ)
            return;

        auto offset = (tick_value - base_);

        asks_[start_ + offset].volume += order_volume_getter(order);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);

        auto offset = (tick_value - base_);
        auto current = bids_[start_ + offset].volume;
        auto volume_delta = supplied_volume - original_volume;

        bids_[start_ + offset].volume += volume_delta;
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_ask(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);

        auto offset = (tick_value - base_);
        auto current = asks_[start_ + offset].volume;
        auto volume_delta = supplied_volume - original_volume;

        asks_[start_ + offset].volume += volume_delta;
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_bid(tick_type tick_value, U&& order)
    {
        std::cout << "REMOVE BID - OrderId: " << order_id_getter(order) << std::endl;
        // TODO: Remove bid from price level
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(tick_type tick_value, U&& order)
    {
        std::cout << "REMOVE ASK - OrderId: " << order_id_getter(order) << std::endl;
        // TODO: Remove ask from price level
    }

    volume_type bid_volume_at_tick(tick_type tick_value)
    {
        auto offset = (tick_value - base_);
        return bids_[start_ + offset].volume;
    }

    volume_type ask_volume_at_tick(tick_type tick_value)
    {
        auto offset = (tick_value - base_);
        return asks_[start_ + offset].volume;
    }

    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] tick_type base() const noexcept { return base_; }
private:
    tick_type base_;
    size_type size_;
    size_type start_{size_ / 2};
    bid_storage bids_;
    ask_storage asks_;
    order_storage orders_;
};

} // namespace jazzy
