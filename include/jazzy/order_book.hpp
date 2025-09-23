#pragma once

#include <jazzy/traits.hpp>

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace jazzy {

template <typename TickType, typename OrderType, size_t Depth = 64>
requires tick<TickType> && order<OrderType>
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

    order_book(tick_type base)
        : base_{std::move(base)}
    {
        bids_.resize(Depth);
        asks_.resize(Depth);
        orders_.reserve(Depth);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_bid(tick_type tick_value, U&& order)
    {
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        if (!succ)
            return;

        auto offset = (tick_value - base_);
        size_t index = start_ + offset;

        if (0 > index || index >= Depth)
            throw std::out_of_range("Tick value out of range for bids");

        bids_[index].volume += order_volume_getter(order);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        if (!succ)
            return;

        auto offset = (tick_value - base_);
        size_t index = start_ + offset;

        if (0 > index || index >= Depth)
            throw std::out_of_range("Tick value out of range for asks");

        asks_[index].volume += order_volume_getter(order);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        if (it == orders_.end())
            throw std::runtime_error("Bid Order not found");

        auto offset = (tick_value - base_);
        auto current = bids_[start_ + offset].volume;
        size_t index = start_ + offset;

        if (0 > index || index >= Depth)
            throw std::out_of_range("Tick value out of range for bids");

        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);

        auto volume_delta = supplied_volume - original_volume;

        bids_[index].volume += volume_delta;
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_ask(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        if (it == orders_.end())
            throw std::runtime_error("Ask Order not found");

        auto offset = (tick_value - base_);
        auto current = asks_[start_ + offset].volume;
        size_t index = start_ + offset;

        if (0 > index || index >= Depth)
            throw std::out_of_range("Tick value out of range for asks");

        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);

        auto volume_delta = supplied_volume - original_volume;

        asks_[index].volume += volume_delta;
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_bid(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);

        if (it != orders_.end())
            orders_.erase(it);
        else
            throw std::runtime_error("Bid Order not found");

        auto offset = (tick_value - base_);
        size_t index = start_ + offset;
        if (0 > index || index >= Depth)
            throw std::out_of_range("Tick value out of range for bids");

        auto current = bids_[start_ + offset].volume;
        bids_[index].volume -= original_volume;
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);

        if (it != orders_.end())
            orders_.erase(it);
        else
            throw std::runtime_error("Ask Order not found");

        auto offset = (tick_value - base_);
        size_t index = start_ + offset;
        if (0 > index || index >= Depth)
            throw std::out_of_range("Tick value out of range for asks");

        auto current = asks_[start_ + offset].volume;
        asks_[index].volume -= original_volume;
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

    [[nodiscard]] size_type size() const noexcept { return Depth; }
    [[nodiscard]] tick_type base() const noexcept { return base_; }
    [[nodiscard]] size_type low() const noexcept { return 0; }
    [[nodiscard]] size_type high() const noexcept { return Depth - 1; }
private:
    tick_type base_;
    size_type start_{Depth / 2};
    bid_storage bids_;
    ask_storage asks_;
    order_storage orders_;
};

} // namespace jazzy
