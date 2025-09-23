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

    order_book(tick_type bid_base_value, tick_type ask_base_value)
        : bid_base_value_{std::move(bid_base_value)}
        , ask_base_value_{std::move(ask_base_value)}
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

        assert(succ && "Duplicate order id on insert_bid");

        size_t index = tick_to_index<true>(tick_value);
        bids_[index].volume += order_volume_getter(order);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        assert(succ && "Duplicate order id on insert_ask");

        size_t index = tick_to_index<false>(tick_value);
        asks_[index].volume += order_volume_getter(order);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        auto it = orders_.find(order_id_getter(order));

        assert(it != orders_.end() && "Bid Order not found");

        size_t index = tick_to_index<true>(tick_value);

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

        assert(it != orders_.end() && "Ask Order not found");

        size_t index = tick_to_index<false>(tick_value);

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
            assert(false && "Bid Order not found");

        size_t index = tick_to_index<true>(tick_value);
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
            assert(false && "Ask Order not found");

        size_t index = tick_to_index<false>(tick_value);
        asks_[index].volume -= original_volume;
    }

    volume_type bid_volume_at_tick(tick_type tick_value)
    {
        size_t index = tick_to_index<true>(tick_value);
        return bids_[index].volume;
    }

    volume_type ask_volume_at_tick(tick_type tick_value)
    {
        size_t index = tick_to_index<false>(tick_value);
        return asks_[index].volume;
    }

    [[nodiscard]] volume_type bid_volume_at_level(size_type level) const
    {
        assert(level < Depth);

        // scanning from high to low
        // so level 0 is the highest bid
        // level 1 is the next highest bid, etc
        for (size_t start_ = Depth - 1; start_ > 0; --start_)
        {
            if (bids_[start_].volume != 0)
            {
                if (level == 0)
                    return bids_[start_].volume;
                --level;
            }
        }
        return 0;
    }

    [[nodiscard]] volume_type ask_volume_at_level(size_type level) const
    {
        assert(level < Depth);

        // scanning from low to high
        // so level 0 is the lowest ask
        // level 1 is the next lowest ask, etc
        for (size_t start_ = 0; start_ < Depth; ++start_)
        {
            if (asks_[start_].volume != 0)
            {
                if (level == 0)
                    return asks_[start_].volume;
                --level;
            }
        }
        return 0;
    }

    [[nodiscard]] size_type constexpr size() const noexcept { return Depth; }
    [[nodiscard]] tick_type bid_base_value() const noexcept { return bid_base_value_; }
    [[nodiscard]] tick_type ask_base_value() const noexcept { return ask_base_value_; }
    [[nodiscard]] size_type constexpr low() const noexcept { return 0; }
    [[nodiscard]] size_type constexpr high() const noexcept { return Depth - 1; }
private:
    template <bool is_bid>
    constexpr size_t tick_to_index(tick_type tick_value) const // pass by value is integral
    {
        if constexpr (is_bid)
        {
            int offset = static_cast<int>(bid_base_value_) - static_cast<int>(tick_value);
            assert(offset >= 0 && offset < static_cast<int>(Depth) && "Tick value out of range for bids");
            size_t index = (Depth - 1) - offset; // Start from high end for bids
            assert(index < Depth);
            return index;
        }
        else
        {
            int offset = static_cast<int>(tick_value) - static_cast<int>(ask_base_value_);
            assert(offset >= 0 && offset < static_cast<int>(Depth) && "Tick value out of range for asks");
            size_t index = offset; // Start from low end for asks
            assert(index < Depth);
            return index;
        }
    }
    tick_type bid_base_value_; // represents the highest possible value of the bids
    tick_type ask_base_value_; // represents the lowest possible value of the asks

    bid_storage bids_;
    ask_storage asks_;
    order_storage orders_;
};

} // namespace jazzy
