#pragma once

#include <cassert>
#include <jazzy/traits.hpp>
#include <jazzy/types.hpp>

#include <map>
#include <unordered_map>

namespace jazzy { namespace benchmarks {

template <typename TickType, typename OrderType, typename MarketStats>
requires tick<TickType> && order<OrderType> && market_stats<MarketStats> &&
    std::same_as<typename MarketStats::tick_type, TickType>
class map_order_book
{
    using order_type = OrderType;
    using tick_type = TickType;
    using base_type = TickType;
    using id_type = order_id_type<order_type>;
    using volume_type = order_volume_type<order_type>;

    static constexpr tick_type NO_BID_VALUE = std::numeric_limits<tick_type>::min();
    static constexpr tick_type NO_ASK_VALUE = std::numeric_limits<tick_type>::max();

    struct level
    {
        volume_type volume{};
    };

private:
    using bid_storage = std::map<tick_type, level, std::greater<tick_type>>;
    using ask_storage = std::map<tick_type, level, std::less<tick_type>>;
    using order_storage = std::unordered_map<id_type, order_type>;

public:
    using value_type = OrderType;
    using size_type = size_t;

    map_order_book()
    {
        constexpr size_t estimated_levels =
            static_cast<size_t>(MarketStats::daily_high_v.value() - MarketStats::daily_low_v.value());
        orders_.reserve(estimated_levels * 10); // Arbitrary factor to reduce rehashing
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_bid(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return; // Ignore out of range bids

        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        assert(succ);

        order_tick_setter(it->second, tick_value);

        bids_[tick_value].volume += order_volume_getter(order);

        best_bid_ = std::max(best_bid_, tick_value);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return; // Ignore out of range asks

        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        assert(succ);

        order_tick_setter(it->second, tick_value);

        asks_[tick_value].volume += order_volume_getter(order);

        best_ask_ = std::min(best_ask_, tick_value);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        // ignore out of range bids
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return;

        auto it = orders_.find(order_id_getter(order));
        assert(it != orders_.end());

        auto original_tick = order_tick_getter(it->second);
        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);
        order_tick_setter(it->second, tick_value);

        if (tick_value == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            bids_[tick_value].volume += volume_delta;

            // Remove level if volume reaches zero
            if (bids_[tick_value].volume == 0)
            {
                bids_.erase(tick_value);

                // Update best bid if needed
                if (original_tick == best_bid_)
                {
                    best_bid_ = find_best_bid();
                }
            }
        }
        else
        {
            // Remove volume from old level
            bids_[original_tick].volume -= original_volume;
            if (bids_[original_tick].volume == 0)
            {
                bids_.erase(original_tick);
            }

            // Add volume to new level
            bids_[tick_value].volume += supplied_volume;

            // Update best_bid if needed
            if (tick_value > best_bid_)
            {
                best_bid_ = tick_value;
            }
            else if (original_tick == best_bid_)
            {
                best_bid_ = find_best_bid();
            }
        }
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_ask(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return; // Ignore out of range asks

        auto it = orders_.find(order_id_getter(order));
        assert(it != orders_.end());

        auto original_tick = order_tick_getter(it->second);
        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);
        order_tick_setter(it->second, tick_value);

        if (tick_value == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            asks_[tick_value].volume += volume_delta;

            // Remove level if volume reaches zero
            if (asks_[tick_value].volume == 0)
            {
                asks_.erase(tick_value);

                // Update best ask if needed
                if (original_tick == best_ask_)
                {
                    best_ask_ = find_best_ask();
                }
            }
        }
        else
        {
            // Remove volume from old level
            asks_[original_tick].volume -= original_volume;
            if (asks_[original_tick].volume == 0)
            {
                asks_.erase(original_tick);
            }

            // Add volume to new level
            asks_[tick_value].volume += supplied_volume;

            // Update best_ask if needed
            if (tick_value < best_ask_)
            {
                best_ask_ = tick_value;
            }
            else if (original_tick == best_ask_)
            {
                best_ask_ = find_best_ask();
            }
        }
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_bid(tick_type tick_value, U&& order)
    {
        // Ignore out of range bids
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return;

        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);

        assert(it != orders_.end());
        orders_.erase(it);

        bids_[tick_value].volume -= original_volume;
        if (bids_[tick_value].volume == 0)
        {
            bids_.erase(tick_value);

            // Update best bid if needed
            if (tick_value == best_bid_)
            {
                best_bid_ = find_best_bid();
            }
        }
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(tick_type tick_value, U&& order)
    {
        // Ignore out of range asks
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return;

        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);

        assert(it != orders_.end());
        orders_.erase(it);

        asks_[tick_value].volume -= original_volume;
        if (asks_[tick_value].volume == 0)
        {
            asks_.erase(tick_value);

            // Update best ask if needed
            if (tick_value == best_ask_)
            {
                best_ask_ = find_best_ask();
            }
        }
    }

    volume_type bid_volume_at_tick(tick_type tick_value)
    {
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return 0; // Out of range bids have zero volume

        auto it = bids_.find(tick_value);
        return (it != bids_.end()) ? it->second.volume : 0;
    }

    volume_type ask_volume_at_tick(tick_type tick_value)
    {
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return 0; // Out of range asks have zero volume

        auto it = asks_.find(tick_value);
        return (it != asks_.end()) ? it->second.volume : 0;
    }

    [[nodiscard]] order_type bid_at_level(size_type level) const
    {
        // scanning from best bid to low
        // so level 0 is the highest bid
        // level 1 is the next highest bid, etc
        if (bids_.empty())
        {
            // No bids exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        auto it = bids_.begin();
        for (size_type i = 0; i < level && it != bids_.end(); ++i, ++it)
        {}

        if (it != bids_.end())
        {
            order_type dummy_order{};
            order_volume_setter(dummy_order, it->second.volume);
            order_tick_setter(dummy_order, it->first);
            return dummy_order;
        }

        order_type dummy_order{};
        order_volume_setter(dummy_order, 0);
        order_tick_setter(dummy_order, tick_type{});
        return dummy_order;
    }

    [[nodiscard]] order_type ask_at_level(size_type level) const
    {
        // scanning from best ask to high
        // so level 0 is the lowest ask
        // level 1 is the next lowest ask, etc
        if (asks_.empty())
        {
            // No asks exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        auto it = asks_.begin();
        for (size_type i = 0; i < level && it != asks_.end(); ++i, ++it)
        {}

        if (it != asks_.end())
        {
            order_type dummy_order{};
            order_volume_setter(dummy_order, it->second.volume);
            order_tick_setter(dummy_order, it->first);
            return dummy_order;
        }

        order_type dummy_order{};
        order_volume_setter(dummy_order, 0);
        order_tick_setter(dummy_order, tick_type{});
        return dummy_order;
    }

    [[nodiscard]] size_type size() const noexcept
    {
        // Return the theoretical maximum size based on market range
        return static_cast<size_t>(MarketStats::daily_high_v.value() - MarketStats::daily_low_v.value()) *
            (1.0 + MarketStats::expected_range_v);
    }

    [[nodiscard]] size_type low() const noexcept { return 0; }
    [[nodiscard]] size_type high() const noexcept { return size() - 1; }

    [[nodiscard]] tick_type best_bid() const noexcept { return best_bid_; }
    [[nodiscard]] tick_type best_ask() const noexcept { return best_ask_; }

private:
    tick_type find_best_bid() const { return bids_.empty() ? NO_BID_VALUE : bids_.begin()->first; }

    tick_type find_best_ask() const { return asks_.empty() ? NO_ASK_VALUE : asks_.begin()->first; }

    tick_type best_bid_{NO_BID_VALUE};
    tick_type best_ask_{NO_ASK_VALUE};
    bid_storage bids_;
    ask_storage asks_;
    order_storage orders_;
};
}} // namespace jazzy::benchmarks