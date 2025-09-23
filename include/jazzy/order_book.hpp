#pragma once

#include <cassert>
#include <jazzy/traits.hpp>
#include <jazzy/types.hpp>

#include <bitset>
#include <unordered_map>
#include <vector>

namespace jazzy {

template <typename TickType, typename OrderType, typename MarketStats>
requires tick<TickType> && order<OrderType> && market_stats<MarketStats> &&
    std::same_as<typename MarketStats::tick_type, TickType>
class order_book
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
    static constexpr size_t calculate_size(tick_type daily_high, tick_type daily_low, double expected_range)
    {
        return static_cast<size_t>(daily_high - daily_low) * (1.0 + expected_range);
    }

    using bid_storage = std::vector<level>;
    using ask_storage = std::vector<level>;
    using order_storage = std::unordered_map<id_type, order_type>;

public:
    using value_type = OrderType;
    using size_type = size_t;
    using bitset_type =
        std::bitset<calculate_size(MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::expected_range_v)>;

    order_book()
    {
        bids_.resize(size_);
        asks_.resize(size_);
        orders_.reserve(size_ * 10); // Arbitrary factor to reduce rehashing
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_bid(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
            return; // Ignore out of range bids

        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        assert(succ);

        order_tick_setter(it->second, tick_value);

        size_type index = tick_to_index<true>(tick_value);
        bids_[index].volume += order_volume_getter(order);

        best_bid_ = std::max(best_bid_, tick_value);

        update_bitsets<true>(true, index);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
            return; // Ignore out of range asks

        auto [it, succ] = orders_.try_emplace(order_id_getter(order), order);

        assert(succ);

        order_tick_setter(it->second, tick_value);

        size_type index = tick_to_index<false>(tick_value);
        asks_[index].volume += order_volume_getter(order);

        best_ask_ = std::min(best_ask_, tick_value);

        update_bitsets<false>(true, index);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        // ignore out of range bids
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
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
            size_type index = tick_to_index<true>(tick_value);
            bids_[index].volume += volume_delta;

            // Only scan if we might have removed the best bid
            if (volume_delta < 0 && original_tick == best_bid_ && bids_[index].volume == 0)
            {
                best_bid_ = scan_for_best_bid(index);

                update_bitsets<true>(false, index);
            }
        }
        else
        {
            size_type old_index = tick_to_index<true>(original_tick);
            bids_[old_index].volume -= original_volume;

            size_type new_index = tick_to_index<true>(tick_value);
            bids_[new_index].volume += supplied_volume;

            // Update best_bid if needed
            if (tick_value > best_bid_)
            {
                best_bid_ = tick_value;
            }
            else if (original_tick == best_bid_ && bids_[old_index].volume == 0)
            {
                best_bid_ = scan_for_best_bid(old_index);

                update_bitsets<true>(false, old_index);
            }
        }
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_ask(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
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
            size_type index = tick_to_index<false>(tick_value);
            asks_[index].volume += volume_delta;

            // Only scan if we might have removed the best ask
            if (volume_delta < 0 && original_tick == best_ask_ && asks_[index].volume == 0)
            {
                best_ask_ = scan_for_best_ask(index);

                update_bitsets<false>(false, index);
            }
        }
        else
        {
            size_type old_index = tick_to_index<false>(original_tick);
            asks_[old_index].volume -= original_volume;

            size_type new_index = tick_to_index<false>(tick_value);
            asks_[new_index].volume += supplied_volume;

            // Update best_ask if needed
            if (tick_value < best_ask_)
            {
                best_ask_ = tick_value;
            }
            else if (original_tick == best_ask_ && asks_[old_index].volume == 0)
            {
                best_ask_ = scan_for_best_ask(old_index);

                update_bitsets<false>(false, old_index);
            }
        }
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_bid(tick_type tick_value, U&& order)
    {
        // Ignore out of range bids
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
            return;

        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);

        assert(it != orders_.end());
        orders_.erase(it);

        size_type index = tick_to_index<true>(tick_value);
        bids_[index].volume -= original_volume;
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(tick_type tick_value, U&& order)
    {
        // Ignore out of range asks
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
            return;

        auto it = orders_.find(order_id_getter(order));

        auto original_volume = order_volume_getter(it->second);

        assert(it != orders_.end());
        orders_.erase(it);

        size_type index = tick_to_index<false>(tick_value);
        asks_[index].volume -= original_volume;
    }

    volume_type bid_volume_at_tick(tick_type tick_value)
    {
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
            return 0; // Out of range bids have zero volume

        size_type index = tick_to_index<true>(tick_value);
        return bids_[index].volume;
    }

    volume_type ask_volume_at_tick(tick_type tick_value)
    {
        if (tick_value > MarketStats::daily_high_v || tick_value < MarketStats::daily_low_v)
            return 0; // Out of range asks have zero volume

        size_type index = tick_to_index<false>(tick_value);
        return asks_[index].volume;
    }

    [[nodiscard]] order_type bid_at_level(size_type level) const
    {
        assert(level < size_);

        // scanning from best bid to low
        // so level 0 is the highest bid
        // level 1 is the next highest bid, etc
        if (best_bid_ == NO_BID_VALUE)
        {
            // No bids exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        for (size_type i = tick_to_index<true>(best_bid_);; --i)
        {
            if (bids_[i].volume != 0)
            {
                if (level == 0)
                {
                    order_type dummy_order{};
                    order_volume_setter(dummy_order, bids_[i].volume);
                    order_tick_setter(dummy_order, index_to_tick<true>(i));
                    return dummy_order;
                }
                --level;
            }
            if (i == 0)
                break;
        }
        order_type dummy_order{};
        order_volume_setter(dummy_order, 0);
        order_tick_setter(dummy_order, tick_type{});
        return dummy_order;
    }

    [[nodiscard]] order_type ask_at_level(size_type level) const
    {
        assert(level < size_);

        // scanning from best ask to high
        // so level 0 is the lowest ask
        // level 1 is the next lowest ask, etc
        if (best_ask_ == NO_ASK_VALUE)
        {
            // No asks exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        for (size_type i = tick_to_index<false>(best_ask_); i < size_; ++i)
        {
            if (asks_[i].volume != 0)
            {
                if (level == 0)
                {
                    order_type dummy_order{};
                    order_volume_setter(dummy_order, asks_[i].volume);
                    order_tick_setter(dummy_order, index_to_tick<false>(i));
                    return dummy_order;
                }
                --level;
            }
        }
        order_type dummy_order{};
        order_volume_setter(dummy_order, 0);
        order_tick_setter(dummy_order, tick_type{});
        return dummy_order;
    }

    [[nodiscard]] size_type constexpr size() const noexcept { return size_; }
    [[nodiscard]] size_type constexpr low() const noexcept { return 0; }
    [[nodiscard]] size_type constexpr high() const noexcept { return size_ - 1; }

    [[nodiscard]] tick_type best_bid() const noexcept { return best_bid_; }
    [[nodiscard]] tick_type best_ask() const noexcept { return best_ask_; }

    [[nodiscard]] const bitset_type& bid_bitmap() const noexcept { return bid_bitmap_; }
    [[nodiscard]] const bitset_type& ask_bitmap() const noexcept { return ask_bitmap_; }

private:
    template <bool is_bid>
    constexpr size_type tick_to_index(tick_type tick_value) const
    {
        if constexpr (is_bid)
        {
            auto high_val = static_cast<unsigned long>(MarketStats::daily_high_v);
            auto tick_val = static_cast<unsigned long>(tick_value);
            assert(tick_val <= high_val && "Tick value above daily high for bids");
            size_type offset = high_val - tick_val;
            assert(offset < size_ && "Tick value out of range for bids");
            return (size_ - 1) - offset; // Start from high end for bids
        }
        else
        {
            size_type index = static_cast<unsigned long>(tick_value) -
                static_cast<unsigned long>(MarketStats::daily_low_v); // Start from low end for asks
            assert(index >= 0 && index < size_ && "Tick value out of range for asks");
            return index;
        }
    }

    template <bool is_bid>
    constexpr tick_type index_to_tick(size_type index) const
    {
        assert(index < size_ && "Index out of range");
        if constexpr (is_bid)
        {
            return static_cast<tick_type>(MarketStats::daily_high_v - static_cast<base_type>((size_ - 1) - index));
        }
        else
        {
            return static_cast<tick_type>(MarketStats::daily_low_v + static_cast<base_type>(index));
        }
    }

    tick_type scan_for_best_bid(size_type start_index) const
    {
        for (size_type i = start_index;; --i)
        {
            if (bids_[i].volume != 0)
                return index_to_tick<true>(i);
            if (i == 0)
                break;
        }
        return NO_BID_VALUE;
    }

    tick_type scan_for_best_ask(size_type start_index) const
    {
        for (size_type i = start_index; i < size_; ++i)
        {
            if (asks_[i].volume != 0)
                return index_to_tick<false>(i);
        }
        return NO_ASK_VALUE;
    }

    template <bool is_bid>
    void update_bitsets(bool on, size_type index)
    {
        if constexpr (is_bid)
        {
            bid_bitmap_.set(index, on);
        }
        else
        {
            ask_bitmap_.set(index, on);
        }
    }

    static constexpr size_type size_ =
        calculate_size(MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::expected_range_v);
    tick_type best_bid_{NO_BID_VALUE};
    tick_type best_ask_{NO_ASK_VALUE};
    bid_storage bids_;
    bitset_type bid_bitmap_{};
    ask_storage asks_;
    bitset_type ask_bitmap_{};
    order_storage orders_;
};

} // namespace jazzy
