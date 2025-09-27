#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <jazzy/traits.hpp>
#include <jazzy/types.hpp>

#include <bitset>
#include <unordered_map>
#include <utility>
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

public:
    using value_type = OrderType;
    using size_type = size_t;

private:
    using tick_type_strong = typename MarketStats::tick_type_strong_t;

    static constexpr size_type calculate_size(
        const tick_type_strong& daily_high, const tick_type_strong& daily_low, double expected_range)
    {
        // sanity check: protect against inverted range
        assert(daily_high > daily_low && "Daily high must be greater than daily low");
        assert(expected_range >= 0.0 && "Expected range must be non-negative");

        const auto base_span = static_cast<double>((daily_high - daily_low).value());
        const auto scaled_span = static_cast<size_type>(base_span * (1.0 + expected_range));
        const auto inclusive_span = static_cast<size_type>((daily_high - daily_low).value()) + 1;
        const size_type total_span = scaled_span < inclusive_span ? inclusive_span : scaled_span;
        assert(total_span > 0 && "Order book size must be positive for valid bitsets");
        return total_span;
    }

    static constexpr std::pair<std::int64_t, std::int64_t> calculate_bounds(
        const tick_type_strong& daily_high,
        const tick_type_strong& daily_low,
        const tick_type_strong& close,
        double expected_range)
    {
        const auto span = static_cast<std::int64_t>(calculate_size(daily_high, daily_low, expected_range));
        const auto base_low = static_cast<std::int64_t>(daily_low.value());
        const auto base_high = static_cast<std::int64_t>(daily_high.value());
        const auto center = static_cast<std::int64_t>(close.value());

        std::int64_t lower = center - (span / 2);
        std::int64_t upper = lower + span - 1;

        if (upper < base_high)
        {
            const auto shift = base_high - upper;
            lower += shift;
            upper += shift;
        }

        if (lower > base_low)
        {
            const auto shift = lower - base_low;
            lower -= shift;
            upper -= shift;
        }

        return {lower, upper};
    }

    static constexpr tick_type_strong calculate_lower_bound(
        const tick_type_strong& daily_high,
        const tick_type_strong& daily_low,
        const tick_type_strong& close,
        double expected_range)
    {
        const auto bounds = calculate_bounds(daily_high, daily_low, close, expected_range);
        return tick_type_strong{static_cast<TickType>(bounds.first)};
    }

    static constexpr tick_type_strong calculate_upper_bound(
        const tick_type_strong& daily_high,
        const tick_type_strong& daily_low,
        const tick_type_strong& close,
        double expected_range)
    {
        const auto bounds = calculate_bounds(daily_high, daily_low, close, expected_range);
        return tick_type_strong{static_cast<TickType>(bounds.second)};
    }

    using bid_storage = std::vector<level>;
    using ask_storage = std::vector<level>;
    using order_storage = std::unordered_map<id_type, order_type>;

public:
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
        // wrap in strong type to avoid accidental misuse
        tick_type_strong tick_strong{tick_value};
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return; // Ignore out of range bids

        auto volume = order_volume_getter(order);
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), std::forward<U>(order));

        assert(succ);

        order_tick_setter(it->second, tick_strong.value());

        size_type index = tick_to_index(tick_strong);
        bids_[index].volume += volume;

        if (!best_bid_.has_value() || tick_strong > best_bid_)
            best_bid_ = tick_strong;

        update_bitsets<true>(true, index);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        tick_type_strong tick_strong{tick_value};
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return; // Ignore out of range asks

        auto volume = order_volume_getter(order);
        auto [it, succ] = orders_.try_emplace(order_id_getter(order), std::forward<U>(order));

        assert(succ);

        order_tick_setter(it->second, tick_strong.value());

        size_type index = tick_to_index(tick_strong);
        asks_[index].volume += volume;

        if (!best_ask_.has_value() || tick_strong < best_ask_)
            best_ask_ = tick_strong;

        update_bitsets<false>(true, index);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        tick_type_strong tick_strong{tick_value};
        // ignore out of range bids
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return;

        auto it = orders_.find(order_id_getter(order));
        assert(it != orders_.end());

        auto original_tick = tick_type_strong(order_tick_getter(it->second));
        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);
        order_tick_setter(it->second, tick_strong.value());

        if (tick_strong == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            size_type index = tick_to_index(tick_strong);
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
            size_type old_index = tick_to_index(original_tick);
            bids_[old_index].volume -= original_volume;

            size_type new_index = tick_to_index(tick_strong);
            bids_[new_index].volume += supplied_volume;

            // Update best_bid if needed
            if (tick_strong > best_bid_)
            {
                best_bid_ = tick_strong;
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
        tick_type_strong tick_strong{tick_value};
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return; // Ignore out of range asks

        auto it = orders_.find(order_id_getter(order));
        assert(it != orders_.end());

        auto original_tick = tick_type_strong(order_tick_getter(it->second));
        auto original_volume = order_volume_getter(it->second);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(it->second, supplied_volume);
        order_tick_setter(it->second, tick_strong.value());

        if (tick_strong == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            size_type index = tick_to_index(tick_strong);
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
            size_type old_index = tick_to_index(original_tick);
            asks_[old_index].volume -= original_volume;

            size_type new_index = tick_to_index(tick_strong);
            asks_[new_index].volume += supplied_volume;

            // Update best_ask if needed
            if (tick_strong < best_ask_)
            {
                best_ask_ = tick_strong;
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
        tick_type_strong tick_strong{tick_value};
        // Ignore out of range bids
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return;

        auto it = orders_.find(order_id_getter(order));
        assert(it != orders_.end());

        auto original_volume = order_volume_getter(it->second);

        assert(it != orders_.end());
        orders_.erase(it);

        size_type index = tick_to_index(tick_strong);
        bids_[index].volume -= original_volume;

        // update best_bid_ if needed
        if (bids_[index].volume == 0)
        {
            update_bitsets<true>(false, index);
            if (best_bid_.has_value() && tick_strong == best_bid_)
            {
                best_bid_ = scan_for_best_bid(index == 0 ? size_type{0} : index - 1);
            }
        }
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(tick_type tick_value, U&& order)
    {
        tick_type_strong tick_strong{tick_value};
        // Ignore out of range asks
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return;

        auto it = orders_.find(order_id_getter(order));
        assert(it != orders_.end());

        auto original_volume = order_volume_getter(it->second);

        assert(it != orders_.end());
        orders_.erase(it);

        size_type index = tick_to_index(tick_strong);
        asks_[index].volume -= original_volume;

        // update best_ask_ if needed
        if (asks_[index].volume == 0)
        {
            update_bitsets<false>(false, index);
            if (best_ask_.has_value() && tick_strong == best_ask_)
            {
                best_ask_ = scan_for_best_ask(index + 1 < size_ ? index + 1 : size_ - 1);
            }
        }
    }

    volume_type bid_volume_at_tick(tick_type tick_value)
    {
        tick_type_strong tick_strong{tick_value};
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return 0; // Out of range bids have zero volume

        size_type index = tick_to_index(tick_strong);
        return bids_[index].volume;
    }

    volume_type ask_volume_at_tick(tick_type tick_value)
    {
        tick_type_strong tick_strong{tick_value};
        if (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v)
            return 0; // Out of range asks have zero volume

        size_type index = tick_to_index(tick_strong);
        return asks_[index].volume;
    }

    [[nodiscard]] order_type bid_at_level(size_type level) const
    {
        assert(level < size_);

        // scanning from best bid to low
        // so level 0 is the highest bid
        // level 1 is the next highest bid, etc
        if (!best_bid_.has_value())
        {
            // No bids exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        for (size_type i = tick_to_index(best_bid_);; --i)
        {
            if (bids_[i].volume != 0)
            {
                if (level == 0)
                {
                    order_type dummy_order{};
                    order_volume_setter(dummy_order, bids_[i].volume);
                    order_tick_setter(dummy_order, index_to_tick(i).value());
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
        if (!best_ask_.has_value())
        {
            // No asks exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        for (size_type i = tick_to_index(best_ask_); i < size_; ++i)
        {
            if (asks_[i].volume != 0)
            {
                if (level == 0)
                {
                    order_type dummy_order{};
                    order_volume_setter(dummy_order, asks_[i].volume);
                    order_tick_setter(dummy_order, index_to_tick(i).value());
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

    [[nodiscard]] tick_type best_bid() const noexcept
    {
        return best_bid_.has_value() ? best_bid_.value() : NO_BID_VALUE;
    }
    [[nodiscard]] tick_type best_ask() const noexcept
    {
        return best_ask_.has_value() ? best_ask_.value() : NO_ASK_VALUE;
    }

    [[nodiscard]] const bitset_type& bid_bitmap() const noexcept { return bid_bitmap_; }
    [[nodiscard]] const bitset_type& ask_bitmap() const noexcept { return ask_bitmap_; }

    constexpr size_type tick_to_index(const tick_type_strong& tick_value) const
    {
        const auto tick_val = static_cast<std::int64_t>(tick_value.value());
        const auto low_val = static_cast<std::int64_t>(range_low_v_.value());
        assert(tick_value <= range_high_v_ && tick_value >= range_low_v_ && "Tick value out of range for price level");
        const auto offset = tick_val - low_val;
        assert(offset >= 0 && static_cast<size_type>(offset) < size_ && "Tick value out of range for price level");
        return static_cast<size_type>(offset);
    }

    constexpr tick_type_strong index_to_tick(size_type index) const
    {
        assert(index < size_ && "Index out of range");
        return tick_type_strong{static_cast<tick_type>(range_low_v_.value() + static_cast<base_type>(index))};
    }

    tick_type_strong scan_for_best_bid(size_type start_index) const
    {
        for (size_type i = start_index;; --i)
        {
            if (bids_[i].volume != 0)
                return index_to_tick(i);
            if (i == 0)
                break;
        }
        return tick_type_strong::no_value();
    }

    tick_type_strong scan_for_best_ask(size_type start_index) const
    {
        for (size_type i = start_index; i < size_; ++i)
        {
            if (asks_[i].volume != 0)
                return index_to_tick(i);
        }
        return tick_type_strong::no_value();
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
    static constexpr tick_type_strong range_low_v_ = calculate_lower_bound(
        MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::daily_close_v, MarketStats::expected_range_v);
    static constexpr tick_type_strong range_high_v_ = calculate_upper_bound(
        MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::daily_close_v, MarketStats::expected_range_v);
    tick_type_strong best_bid_ = tick_type_strong::no_value();
    tick_type_strong best_ask_ = tick_type_strong::no_value();
    bid_storage bids_;
    bitset_type bid_bitmap_{};
    ask_storage asks_;
    bitset_type ask_bitmap_{};
    order_storage orders_;
};

} // namespace jazzy
