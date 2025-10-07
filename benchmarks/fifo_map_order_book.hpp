#pragma once

#include <cassert>
#include <jazzy/traits.hpp>
#include <jazzy/types.hpp>

#include <limits>
#include <list>
#include <map>
#include <unordered_map>

namespace jazzy { namespace benchmarks {

template <typename TickType, typename OrderType, typename MarketStats>
requires tick<TickType> && order<OrderType> && market_stats<MarketStats> &&
    std::same_as<typename MarketStats::tick_type, TickType>
class fifo_map_order_book
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
        std::list<id_type> fifo;
    };

    using queue_iterator = typename std::list<id_type>::iterator;

    struct order_data
    {
        order_type order{};
        queue_iterator queue_it{};
        bool has_queue_it{false};
        bool is_bid{false};
    };

private:
    using bid_storage = std::map<tick_type, level, std::greater<tick_type>>;
    using ask_storage = std::map<tick_type, level, std::less<tick_type>>;
    using order_storage = std::unordered_map<id_type, order_data>;

public:
    using value_type = OrderType;
    using size_type = size_t;

    fifo_map_order_book()
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

        const auto order_id = order_id_getter(order);
        auto [it, succ] = orders_.try_emplace(order_id);

        assert(succ);

        auto& stored_order = it->second;
        stored_order.order = order;
        stored_order.is_bid = true;

        auto [level_it, _] = bids_.try_emplace(tick_value);
        auto& level = level_it->second;
        level.volume += order_volume_getter(order);
        stored_order.queue_it = level.fifo.insert(level.fifo.end(), order_id);
        stored_order.has_queue_it = true;

        order_tick_setter(stored_order.order, tick_value);

        best_bid_ = std::max(best_bid_, tick_value);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return; // Ignore out of range asks

        const auto order_id = order_id_getter(order);
        auto [it, succ] = orders_.try_emplace(order_id);

        assert(succ);

        auto& stored_order = it->second;
        stored_order.order = order;
        stored_order.is_bid = false;

        auto [level_it, _] = asks_.try_emplace(tick_value);
        auto& level = level_it->second;
        level.volume += order_volume_getter(order);
        stored_order.queue_it = level.fifo.insert(level.fifo.end(), order_id);
        stored_order.has_queue_it = true;

        order_tick_setter(stored_order.order, tick_value);

        best_ask_ = std::min(best_ask_, tick_value);
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        // ignore out of range bids
        if (tick_value > MarketStats::daily_high_v.value() || tick_value < MarketStats::daily_low_v.value())
            return;

        const auto order_id = order_id_getter(order);
        auto it = orders_.find(order_id);
        assert(it != orders_.end());

        auto& stored_order = it->second;
        assert(stored_order.is_bid);

        auto original_tick = order_tick_getter(stored_order.order);
        auto original_volume = order_volume_getter(stored_order.order);
        auto supplied_volume = order_volume_getter(order);

        // Save queue iterator before any modifications
        const bool had_queue_it = stored_order.has_queue_it;
        const auto saved_queue_it = stored_order.queue_it;

        order_volume_setter(stored_order.order, supplied_volume);
        order_tick_setter(stored_order.order, tick_value);

        if (tick_value == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            auto level_it = bids_.find(tick_value);
            assert(level_it != bids_.end());
            auto& level = level_it->second;
            level.volume += volume_delta;

            // If volume becomes zero, remove from queue
            if (supplied_volume == 0 && had_queue_it)
            {
                level.fifo.erase(saved_queue_it);
                stored_order.has_queue_it = false;
            }
            // If volume increased, move order to back of the queue (lose priority)
            else if (volume_delta > 0 && had_queue_it)
            {
                level.fifo.erase(saved_queue_it);
                stored_order.queue_it = level.fifo.insert(level.fifo.end(), order_id);
            }
            // If volume decreased (but not to zero), keep position in queue

            // Remove level if volume reaches zero
            if (level.volume == 0)
            {
                bids_.erase(level_it);
                if (original_tick == best_bid_)
                {
                    best_bid_ = find_best_bid();
                }
            }
        }
        else
        {
            // Remove volume from old level
            auto level_it = bids_.find(original_tick);
            assert(level_it != bids_.end());
            auto& current_level = level_it->second;
            current_level.volume -= original_volume;
            if (had_queue_it)
            {
                current_level.fifo.erase(saved_queue_it);
                stored_order.has_queue_it = false;
            }
            if (current_level.volume == 0)
            {
                bids_.erase(level_it);
            }

            // Add volume to new level
            auto [new_level_it, _] = bids_.try_emplace(tick_value);
            auto& new_level = new_level_it->second;
            new_level.volume += supplied_volume;
            stored_order.queue_it = new_level.fifo.insert(new_level.fifo.end(), order_id);
            stored_order.has_queue_it = true;

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

        const auto order_id = order_id_getter(order);
        auto it = orders_.find(order_id);
        assert(it != orders_.end());

        auto& stored_order = it->second;
        assert(!stored_order.is_bid);

        auto original_tick = order_tick_getter(stored_order.order);
        auto original_volume = order_volume_getter(stored_order.order);
        auto supplied_volume = order_volume_getter(order);

        // Save queue iterator before any modifications
        const bool had_queue_it = stored_order.has_queue_it;
        const auto saved_queue_it = stored_order.queue_it;

        order_volume_setter(stored_order.order, supplied_volume);
        order_tick_setter(stored_order.order, tick_value);

        if (tick_value == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            auto level_it = asks_.find(tick_value);
            assert(level_it != asks_.end());
            auto& level = level_it->second;
            level.volume += volume_delta;

            // If volume becomes zero, remove from queue
            if (supplied_volume == 0 && had_queue_it)
            {
                level.fifo.erase(saved_queue_it);
                stored_order.has_queue_it = false;
            }
            // If volume increased, move order to back of the queue (lose priority)
            else if (volume_delta > 0 && had_queue_it)
            {
                level.fifo.erase(saved_queue_it);
                stored_order.queue_it = level.fifo.insert(level.fifo.end(), order_id);
            }
            // If volume decreased (but not to zero), keep position in queue

            // Remove level if volume reaches zero
            if (level.volume == 0)
            {
                asks_.erase(level_it);
                if (original_tick == best_ask_)
                {
                    best_ask_ = find_best_ask();
                }
            }
        }
        else
        {
            // Remove volume from old level
            auto level_it = asks_.find(original_tick);
            assert(level_it != asks_.end());
            auto& current_level = level_it->second;
            current_level.volume -= original_volume;
            if (had_queue_it)
            {
                current_level.fifo.erase(saved_queue_it);
                stored_order.has_queue_it = false;
            }
            if (current_level.volume == 0)
            {
                asks_.erase(level_it);
            }

            // Add volume to new level
            auto [new_level_it, _] = asks_.try_emplace(tick_value);
            auto& new_level = new_level_it->second;
            new_level.volume += supplied_volume;
            stored_order.queue_it = new_level.fifo.insert(new_level.fifo.end(), order_id);
            stored_order.has_queue_it = true;

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
        assert(it != orders_.end());

        auto& stored_order = it->second;
        assert(stored_order.is_bid);
        auto original_volume = order_volume_getter(stored_order.order);
        const bool had_queue_it = stored_order.has_queue_it;
        queue_iterator queue_it = stored_order.queue_it;
        orders_.erase(it);

        auto level_it = bids_.find(tick_value);
        if (level_it != bids_.end())
        {
            auto& level = level_it->second;
            level.volume -= original_volume;
            if (had_queue_it)
            {
                level.fifo.erase(queue_it);
            }

            if (level.volume == 0)
            {
                bids_.erase(level_it);

                // Update best bid if needed
                if (tick_value == best_bid_)
                {
                    best_bid_ = find_best_bid();
                }
            }
        }
        else if (tick_value == best_bid_)
        {
            best_bid_ = find_best_bid();
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
        assert(it != orders_.end());

        auto& stored_order = it->second;
        assert(!stored_order.is_bid);
        auto original_volume = order_volume_getter(stored_order.order);
        const bool had_queue_it = stored_order.has_queue_it;
        queue_iterator queue_it = stored_order.queue_it;
        orders_.erase(it);

        auto level_it = asks_.find(tick_value);
        if (level_it != asks_.end())
        {
            auto& level = level_it->second;
            level.volume -= original_volume;
            if (had_queue_it)
            {
                level.fifo.erase(queue_it);
            }

            if (level.volume == 0)
            {
                asks_.erase(level_it);

                // Update best ask if needed
                if (tick_value == best_ask_)
                {
                    best_ask_ = find_best_ask();
                }
            }
        }
        else if (tick_value == best_ask_)
        {
            best_ask_ = find_best_ask();
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

    [[nodiscard]] order_type front_order_at_bid_level(size_type level) const
    {
        if (bids_.empty())
        {
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        auto it = bids_.begin();
        for (size_type i = 0; i < level && it != bids_.end(); ++i, ++it)
        {}

        if (it != bids_.end() && !it->second.fifo.empty())
        {
            const auto order_id = it->second.fifo.front();
            auto order_it = orders_.find(order_id);
            assert(order_it != orders_.end());
            return order_it->second.order;
        }

        order_type dummy_order{};
        order_volume_setter(dummy_order, 0);
        order_tick_setter(dummy_order, tick_type{});
        return dummy_order;
    }

    [[nodiscard]] order_type front_order_at_ask_level(size_type level) const
    {
        if (asks_.empty())
        {
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        auto it = asks_.begin();
        for (size_type i = 0; i < level && it != asks_.end(); ++i, ++it)
        {}

        if (it != asks_.end() && !it->second.fifo.empty())
        {
            const auto order_id = it->second.fifo.front();
            auto order_it = orders_.find(order_id);
            assert(order_it != orders_.end());
            return order_it->second.order;
        }

        order_type dummy_order{};
        order_volume_setter(dummy_order, 0);
        order_tick_setter(dummy_order, tick_type{});
        return dummy_order;
    }

    [[nodiscard]] size_type size() const noexcept
    {
        // Return the theoretical maximum size based on market range
        const double range = static_cast<double>(MarketStats::daily_high_v.value() - MarketStats::daily_low_v.value());
        const double multiplier = 1.0 + MarketStats::expected_range_v;
        return static_cast<size_t>(range * multiplier);
    }

    [[nodiscard]] size_type low() const noexcept { return 0; }
    [[nodiscard]] size_type high() const noexcept { return size() - 1; }

    [[nodiscard]] tick_type best_bid() const noexcept { return best_bid_; }
    [[nodiscard]] tick_type best_ask() const noexcept { return best_ask_; }

    void clear() noexcept
    {
        bids_.clear();
        asks_.clear();
        orders_.clear();
        best_bid_ = NO_BID_VALUE;
        best_ask_ = NO_ASK_VALUE;
    }

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
