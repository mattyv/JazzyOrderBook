#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <jazzy/traits.hpp>
#include <jazzy/types.hpp>

#include <memory_resource>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <jazzy/detail/level_bitmap.hpp>
#include <jazzy/detail/intrusive_fifo.hpp>
#include <jazzy/detail/level_storage.hpp>
#include <jazzy/detail/zero_volume_policy.hpp>
#include <jazzy/detail/bounds_policy.hpp>

namespace jazzy {

template <
    typename TickType,
    typename OrderType,
    typename MarketStats,
    typename LevelStorage = detail::aggregate_level_storage<OrderType>,
    typename ZeroVolumePolicy = detail::zero_volume_as_valid_policy,
    typename BoundsPolicy = detail::bounds_check_assert_policy>
requires tick<TickType> && order<OrderType> && market_stats<MarketStats> &&
    std::same_as<typename MarketStats::tick_type, TickType>
class order_book
{
    using order_type = OrderType;
    using tick_type = TickType;
    using base_type = TickType;
    using id_type = order_id_type<order_type>;
    using volume_type = order_volume_type<order_type>;
    using level_storage_type = LevelStorage;

    static constexpr tick_type NO_BID_VALUE = std::numeric_limits<tick_type>::min();
    static constexpr tick_type NO_ASK_VALUE = std::numeric_limits<tick_type>::max();
    static constexpr bool is_fifo_enabled = is_fifo_storage_v<level_storage_type>;

    struct level
    {
        volume_type volume{};
        [[no_unique_address]] level_storage_type storage;

        // Default constructor
        level() = default;

        // Copy and move constructors with proper noexcept specifications
        level(const level&) = default;
        level(level&&) noexcept(
            std::is_nothrow_move_constructible_v<volume_type> && std::is_nothrow_move_constructible_v<level_storage_type>) =
            default;
        level& operator=(const level&) = default;
        level& operator=(level&&) noexcept(
            std::is_nothrow_move_assignable_v<volume_type> && std::is_nothrow_move_assignable_v<level_storage_type>) =
            default;

        level(std::pmr::memory_resource* resource)
        requires is_fifo_enabled
            : storage(resource)
        {}
    };

public:
    using value_type = OrderType;
    using size_type = size_t;

private:
    using tick_type_strong = typename MarketStats::tick_type_strong_t;

    // Internal enum for better readability in template calls
    enum class Side { Bid, Ask };
    static constexpr Side BID = Side::Bid;
    static constexpr Side ASK = Side::Ask;

    // When FIFO is enabled, we need to track where each order lives in the deque
    // Use EBO to guarantee zero overhead for aggregate-only orderbooks
    // Empty base for aggregate-only mode
    struct order_data_empty_base
    {};

    // Non-empty base for FIFO mode
    struct order_data_fifo_base
    {
        detail::intrusive_fifo_node<id_type> fifo_node;
    };

    // Each order gets stored with an optional iterator back to its position in the FIFO queue
    // This lets us remove it from the queue in O(1) time when needed
    struct order_data : std::conditional_t<is_fifo_enabled, order_data_fifo_base, order_data_empty_base>
    {
        order_type order;

        order_data() = default;
        explicit order_data(order_type&& o) : order(std::move(o)) {}
        explicit order_data(const order_type& o) : order(o) {}
        order_data(const order_data&) = default;
        order_data(order_data&&) noexcept = default;
        order_data& operator=(const order_data&) = default;
        order_data& operator=(order_data&&) noexcept = default;
    };

    static constexpr size_type calculate_size(
        const tick_type_strong& daily_high, const tick_type_strong& daily_low, double expected_range)
    {
        // sanity check: protect against inverted range
        assert(daily_high > daily_low && "Daily high must be greater than daily low");
        assert(expected_range >= 0.0 && "Expected range must be non-negative");

        const auto base_span = static_cast<double>((daily_high - daily_low).value());
        const auto scaled = base_span * (1.0 + expected_range);

        // Overflow protection: cap at max size_type value
        constexpr auto max_size = std::numeric_limits<size_type>::max();
        if (scaled > static_cast<double>(max_size)) {
            return max_size;
        }

        const auto scaled_span = static_cast<size_type>(scaled);
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

    using bid_storage = std::pmr::vector<level>;
    using ask_storage = std::pmr::vector<level>;
    using order_storage = std::pmr::unordered_map<id_type, order_data, std::hash<id_type>, std::equal_to<id_type>>;

public:
    using bitset_type = detail::level_bitmap<calculate_size(
        MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::expected_range_v)>;

    explicit order_book(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : bid_bitmap_()
        , ask_bitmap_()
        , bids_(resource)
        , asks_(resource)
        , orders_(resource)
        , resource_(resource)
    {
        if constexpr (is_fifo_enabled)
        {
            bids_.reserve(size_);
            asks_.reserve(size_);
            for (size_type idx = 0; idx < size_; ++idx)
            {
                bids_.emplace_back(resource_);
                asks_.emplace_back(resource_);
            }
        }
        else
        {
            bids_.resize(size_);
            asks_.resize(size_);
        }
        orders_.reserve(size_ * 10); // Arbitrary factor to reduce rehashing
    }

    ~order_book() = default;

    order_book(const order_book& other)
        : order_book(other.resource_)
    {
        clone_from(other);
    }

    order_book(order_book&& other) noexcept
        : best_bid_(std::move(other.best_bid_))
        , best_ask_(std::move(other.best_ask_))
        , bid_bitmap_(std::move(other.bid_bitmap_))
        , ask_bitmap_(std::move(other.ask_bitmap_))
        , bids_(std::move(other.bids_))
        , asks_(std::move(other.asks_))
        , orders_(std::move(other.orders_))
        , resource_(other.resource_)
    {}

    order_book& operator=(const order_book& other)
    {
        if (this != &other)
        {
            clone_from(other);
        }
        return *this;
    }

    order_book& operator=(order_book&& other) noexcept
    {
        if (this != &other)
        {
            best_bid_ = std::move(other.best_bid_);
            best_ask_ = std::move(other.best_ask_);
            bid_bitmap_ = std::move(other.bid_bitmap_);
            ask_bitmap_ = std::move(other.ask_bitmap_);
            bids_ = std::move(other.bids_);
            asks_ = std::move(other.asks_);
            orders_ = std::move(other.orders_);
            // Don't change resource_ - preserve destination's resource
        }
        return *this;
    }

    order_book(const order_book& other, std::pmr::memory_resource* resource)
        : order_book(resource)
    {
        clone_from(other);
    }

    void swap(order_book& other) noexcept
    {
        using std::swap;
        swap(best_bid_, other.best_bid_);
        swap(best_ask_, other.best_ask_);
        swap(bid_bitmap_, other.bid_bitmap_);
        swap(ask_bitmap_, other.ask_bitmap_);
        swap(bids_, other.bids_);
        swap(asks_, other.asks_);
        swap(orders_, other.orders_);
        // Don't swap resource_ - containers must deallocate from their original resource
    }

    friend void swap(order_book& lhs, order_book& rhs) noexcept { lhs.swap(rhs); }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_bid(tick_type tick_value, U&& order)
    {
        insert_order_impl<BID>(tick_value, std::forward<U>(order));
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_ask(tick_type tick_value, U&& order)
    {
        insert_order_impl<ASK>(tick_value, std::forward<U>(order));
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_bid(tick_type tick_value, U&& order)
    {
        update_order_impl<BID>(tick_value, std::forward<U>(order));
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_ask(tick_type tick_value, U&& order)
    {
        update_order_impl<ASK>(tick_value, std::forward<U>(order));
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_bid(tick_type tick_value, U&& order)
    {
        remove_order_impl<BID>(tick_value, std::forward<U>(order));
    }

    template <typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_ask(tick_type tick_value, U&& order)
    {
        remove_order_impl<ASK>(tick_value, std::forward<U>(order));
    }

    order_type get_order(id_type id) const
    {
        auto it = orders_.find(id);
        assert(it != orders_.end());
        return it->second.order;
    }

    volume_type bid_volume_at_tick(tick_type tick_value) const
    {
        tick_type_strong tick_strong{tick_value};
        assert(tick_strong <= MarketStats::daily_high_v && tick_strong >= MarketStats::daily_low_v);

        size_type index = tick_to_index(tick_strong);
        return bids_[index].volume;
    }

    volume_type ask_volume_at_tick(tick_type tick_value) const
    {
        tick_type_strong tick_strong{tick_value};
        assert(tick_strong <= MarketStats::daily_high_v && tick_strong >= MarketStats::daily_low_v);

        size_type index = tick_to_index(tick_strong);
        return asks_[index].volume;
    }

    [[nodiscard]] order_type bid_at_level(size_type level) const
    {
        assert(level < size_);

        // scanning from best bid to low
        // so level 0 is the highest bid
        // level 1 is the next highest bid, etc
        const size_type total_levels = static_cast<size_type>(bid_bitmap_.count());
        if (level >= total_levels)
        {
            return order_type{};
        }

        const size_t index = bid_bitmap_.select_from_high(static_cast<size_t>(level));

        order_type order_at_level{};
        order_volume_setter(order_at_level, bids_[index].volume);
        order_tick_setter(order_at_level, index_to_tick(index).value());
        return order_at_level;
    }

    [[nodiscard]] order_type ask_at_level(size_type level) const
    {
        assert(level < size_);

        // scanning from best ask to high
        // so level 0 is the lowest ask
        // level 1 is the next lowest ask, etc
        const size_type total_levels = static_cast<size_type>(ask_bitmap_.count());
        if (level >= total_levels)
        {
            return order_type{};
        }

        const size_t index = ask_bitmap_.select_from_low(static_cast<size_t>(level));

        order_type order_at_level{};
        order_volume_setter(order_at_level, asks_[index].volume);
        order_tick_setter(order_at_level, index_to_tick(index).value());
        return order_at_level;
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

    [[nodiscard]] std::pmr::memory_resource* get_memory_resource() const noexcept { return resource_; }

    void clear() noexcept
    {
        orders_.clear();
        best_bid_ = tick_type_strong::no_value();
        best_ask_ = tick_type_strong::no_value();

        // Clear all levels - simpler than iterating bitmap
        for (size_t i = 0; i < size_; ++i)
        {
            if (bids_[i].volume != 0)
            {
                bids_[i].volume = 0;
                if constexpr (is_fifo_enabled)
                {
                    bids_[i].storage.queue.reset();
                }
                bid_bitmap_.set(i, false);
            }
            if (asks_[i].volume != 0)
            {
                asks_[i].volume = 0;
                if constexpr (is_fifo_enabled)
                {
                    asks_[i].storage.queue.reset();
                }
                ask_bitmap_.set(i, false);
            }
        }
    }

    // FIFO-specific functions: only available when FIFO storage is enabled
    [[nodiscard]] order_type front_order_at_bid_level(size_type level) const
    requires is_fifo_enabled
    {
        assert(level < size_);
        assert(best_bid_.has_value());

        assert(level < static_cast<size_type>(bid_bitmap_.count()));

        const size_t index = bid_bitmap_.select_from_high(static_cast<size_t>(level));
        const auto& queue = bids_[index].storage.queue;
        assert(!queue.empty() && "No orders at this price level");

        return get_order(queue.front().value());
    }

    [[nodiscard]] order_type front_order_at_ask_level(size_type level) const
    requires is_fifo_enabled
    {
        assert(level < size_);
        assert(best_ask_.has_value());

        assert(level < static_cast<size_type>(ask_bitmap_.count()));

        const size_t index = ask_bitmap_.select_from_low(static_cast<size_t>(level));
        const auto& queue = asks_[index].storage.queue;
        assert(!queue.empty() && "No orders at this price level");

        return get_order(queue.front().value());
    }

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

    tick_type_strong scan_for_best_bid() const
    {
        const int index = bid_bitmap_.find_highest();

        return (index < 0) ? tick_type_strong::no_value() : index_to_tick(static_cast<size_type>(index));
    }

    tick_type_strong scan_for_best_ask() const
    {
        const int index = ask_bitmap_.find_lowest();
        return (index < 0) ? tick_type_strong::no_value() : index_to_tick(static_cast<size_type>(index));
    }

    template <Side BookSide>
    void update_bitsets(bool on, size_type index)
    {
        if constexpr (BookSide == BID)
        {
            bid_bitmap_.set(index, on);
        }
        else
        {
            ask_bitmap_.set(index, on);
        }
    }

    // Check if tick is within bounds according to policy
    // Returns true if in bounds, false if out of bounds (discard policy)
    // Asserts if out of bounds with assert policy
    bool check_bounds_policy(tick_type_strong tick) const
    {
        if constexpr (is_bounds_discard_policy_v<BoundsPolicy>)
        {
            return tick <= MarketStats::daily_high_v && tick >= MarketStats::daily_low_v;
        }
        else
        {
            assert(tick <= MarketStats::daily_high_v && tick >= MarketStats::daily_low_v);
            return true;
        }
    }

    // Templated insert implementation for both bids and asks
    template <Side OrderSide, typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void insert_order_impl(tick_type tick_value, U&& order)
    {
        tick_type_strong tick_strong{tick_value};

        auto volume = order_volume_getter(order);
        auto order_id = order_id_getter(order);
        auto [it, succ] = orders_.try_emplace(order_id, order_data{std::forward<U>(order)});

        assert(succ && "Order ID already exists");

        order_tick_setter(it->second.order, tick_strong.value());

        // Check bounds - if out of bounds, keep in map but don't add to price levels
        if (!check_bounds_policy(tick_strong))
            return;

        size_type index = tick_to_index(tick_strong);
        auto& levels = (OrderSide == BID) ? bids_ : asks_;
        auto& best_price = (OrderSide == BID) ? best_bid_ : best_ask_;

        levels[index].volume += volume;

        // If FIFO is enabled, add this order to the back of the queue for this price level
        if constexpr (is_fifo_enabled)
        {
            auto node_lookup = make_node_lookup();
            levels[index].storage.queue.push_back(order_id, node_lookup);
        }

        if constexpr (OrderSide == BID)
        {
            if (!best_price.has_value() || tick_strong > best_price)
                best_price = tick_strong;
        }
        else
        {
            if (!best_price.has_value() || tick_strong < best_price)
                best_price = tick_strong;
        }

        update_bitsets<OrderSide>(true, index);
    }

    // Templated remove implementation for both bids and asks
    template <Side OrderSide, typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void remove_order_impl(tick_type tick_value, U&& order)
    {
        tick_type_strong tick_strong{tick_value};

        const auto order_id = order_id_getter(order);
        auto it = orders_.find(order_id);
        assert(it != orders_.end());

        auto& order_entry = it->second;
        auto original_volume = order_volume_getter(order_entry.order);

        // Check bounds - if out of bounds, just remove from map
        if (!check_bounds_policy(tick_strong))
        {
            orders_.erase(it);
            return;
        }

        size_type index = tick_to_index(tick_strong);
        auto& levels = (OrderSide == BID) ? bids_ : asks_;
        auto& best_price = (OrderSide == BID) ? best_bid_ : best_ask_;

        // Remove from FIFO queue before erasing from map
        if constexpr (is_fifo_enabled)
        {
            auto& node = order_entry.fifo_node;
            if (node.in_queue)
            {
                auto node_lookup = make_node_lookup();
                levels[index].storage.queue.erase(order_id, node_lookup);
            }
        }

        orders_.erase(it);

        levels[index].volume -= original_volume;

        if (levels[index].volume == 0)
        {
            update_bitsets<OrderSide>(false, index);

            if (best_price.has_value() && tick_strong == best_price)
            {
                if constexpr (OrderSide == BID)
                    best_price = scan_for_best_bid();
                else
                    best_price = scan_for_best_ask();
            }
        }
    }

    // Templated update implementation for both bids and asks
    template <Side OrderSide, typename U>
    requires order<U> && std::same_as<order_type, std::decay_t<U>>
    void update_order_impl(tick_type tick_value, U&& order)
    {
        tick_type_strong tick_strong{tick_value};

        auto order_id = order_id_getter(order);
        auto it = orders_.find(order_id);
        assert(it != orders_.end());

        auto& order_entry = it->second;
        auto& stored_order = order_entry.order;

        auto original_tick = tick_type_strong(order_tick_getter(stored_order));
        auto original_volume = order_volume_getter(stored_order);
        auto supplied_volume = order_volume_getter(order);

        order_volume_setter(stored_order, supplied_volume);
        order_tick_setter(stored_order, tick_strong.value());

        // Check bounds for both old and new prices
        bool original_out_of_bounds = false;
        bool new_out_of_bounds = false;

        if constexpr (is_bounds_discard_policy_v<BoundsPolicy>)
        {
            original_out_of_bounds = (original_tick > MarketStats::daily_high_v || original_tick < MarketStats::daily_low_v);
            new_out_of_bounds = (tick_strong > MarketStats::daily_high_v || tick_strong < MarketStats::daily_low_v);

            // If both out of bounds, just update stored order and return
            if (original_out_of_bounds && new_out_of_bounds)
                return;
        }
        else
            assert(tick_strong <= MarketStats::daily_high_v && tick_strong >= MarketStats::daily_low_v);


        const bool price_changed = (tick_strong != original_tick);
        auto& levels = (OrderSide == BID) ? bids_ : asks_;
        auto& best_price = (OrderSide == BID) ? best_bid_ : best_ask_;

        const auto ensure_best_price_on_add = [&](tick_type_strong price) {
            if constexpr (OrderSide == BID)
            {
                if (!best_price.has_value() || price > best_price)
                    best_price = price;
            }
            else
            {
                if (!best_price.has_value() || price < best_price)
                    best_price = price;
            }
        };

        const auto refresh_best_if_empty = [&](tick_type_strong price, size_type index) {
            if (levels[index].volume == 0 && best_price.has_value() && price == best_price)
            {
                if constexpr (OrderSide == BID)
                    best_price = scan_for_best_bid();
                else
                    best_price = scan_for_best_ask();
            }
        };

        const auto sync_bitset = [&](size_type index) {
            update_bitsets<OrderSide>(levels[index].volume != 0, index);
        };

        const auto remove_from_fifo = [&](size_type index) {
            if constexpr (is_fifo_enabled)
            {
                auto& node = order_entry.fifo_node;
                if (node.in_queue)
                {
                    auto node_lookup = make_node_lookup();
                    levels[index].storage.queue.erase(order_id, node_lookup);
                }
            }
        };

        const auto push_to_fifo = [&](size_type index) {
            if constexpr (is_fifo_enabled)
            {
                if (supplied_volume != 0)
                {
                    auto node_lookup = make_node_lookup();
                    levels[index].storage.queue.push_back(order_id, node_lookup);
                }
            }
        };

        const auto move_fifo_to_back = [&](size_type index) {
            if constexpr (is_fifo_enabled)
            {
                auto& node = order_entry.fifo_node;
                if (node.in_queue)
                {
                    auto node_lookup = make_node_lookup();
                    levels[index].storage.queue.move_to_back(order_id, node_lookup);
                }
            }
        };

        // Handle bounds transitions with discard policy
        if constexpr (is_bounds_discard_policy_v<BoundsPolicy>)
        {
            // Case: was out-of-bounds, now in-bounds (behave like insert)
            if (original_out_of_bounds && !new_out_of_bounds)
            {
                size_type new_index = tick_to_index(tick_strong);
                levels[new_index].volume += supplied_volume;

                sync_bitset(new_index);
                push_to_fifo(new_index);
                ensure_best_price_on_add(tick_strong);
                return;
            }

            // Case: was in-bounds, now out-of-bounds (behave like remove from price levels)
            if (!original_out_of_bounds && new_out_of_bounds)
            {
                size_type old_index = tick_to_index(original_tick);

                remove_from_fifo(old_index);
                levels[old_index].volume -= original_volume;
                sync_bitset(old_index);
                refresh_best_if_empty(original_tick, old_index);
                return;
            }
        }

        // Normal case: both in-bounds (or assert policy)
        const make_signed_volume_t<volume_type> volume_delta =
            static_cast<make_signed_volume_t<volume_type>>(supplied_volume) -
            static_cast<make_signed_volume_t<volume_type>>(original_volume);
        const size_type old_index = tick_to_index(original_tick);
        const size_type new_index = tick_to_index(tick_strong);

        // Update old price level
        const auto signed_original = static_cast<make_signed_volume_t<volume_type>>(original_volume);
        const auto volume_adjustment = price_changed ? -signed_original : volume_delta;
        levels[old_index].volume = static_cast<volume_type>(
            static_cast<make_signed_volume_t<volume_type>>(levels[old_index].volume) + volume_adjustment);
        sync_bitset(old_index);

        // Handle zero volume based on policy
        if constexpr (is_zero_volume_delete_policy_v<ZeroVolumePolicy>)
        {
            if (supplied_volume == 0)
            {
                // Delete policy: remove from FIFO queue and erase from orders map
                remove_from_fifo(old_index);
                orders_.erase(it);

                // Update best price if needed
                refresh_best_if_empty(original_tick, old_index);
                return;
            }
        }

        // Handle FIFO queue for old level
        if constexpr (is_fifo_enabled)
        {
            if (price_changed)
            {
                remove_from_fifo(old_index);
            }
            else if (volume_delta > 0)
            {
                move_fifo_to_back(old_index);
            }
        }

        // Update new price level only if price changed
        if (price_changed)
        {
            levels[new_index].volume += supplied_volume;
            sync_bitset(new_index);
            push_to_fifo(new_index);
            ensure_best_price_on_add(tick_strong);
        }

        refresh_best_if_empty(original_tick, old_index);
    }

    template <typename LevelContainer>
    void copy_levels_from(const LevelContainer& source, LevelContainer& target)
    {
        target.clear();
        target.reserve(source.size());
        for (const auto& src_level : source)
        {
            if constexpr (is_fifo_enabled)
            {
                target.emplace_back(resource_);
            }
            else
            {
                target.emplace_back();
            }

            auto& dst_level = target.back();
            dst_level.volume = src_level.volume;

            if constexpr (is_fifo_enabled)
            {
                dst_level.storage = src_level.storage;
            }
        }

        assert(target.size() == source.size());
    }

    void clone_from(const order_book& other)
    {
        best_bid_ = other.best_bid_;
        best_ask_ = other.best_ask_;
        bid_bitmap_ = other.bid_bitmap_;
        ask_bitmap_ = other.ask_bitmap_;

        copy_levels_from(other.bids_, bids_);
        copy_levels_from(other.asks_, asks_);

        orders_.clear();
        const size_type desired_capacity = std::max<size_type>(size_ * 10, other.orders_.size());
        orders_.reserve(desired_capacity);
        for (const auto& [id, data] : other.orders_)
        {
            auto [it, inserted] = orders_.emplace(id, data);
            assert(inserted);
            static_cast<void>(inserted);
        }
    }

    auto make_node_lookup()
    {
        return [this](id_type lookup_id) -> detail::intrusive_fifo_node<id_type>& {
            auto it = orders_.find(lookup_id);
            assert(it != orders_.end());
            return it->second.fifo_node;
        };
    }

    static constexpr size_type size_ =
        calculate_size(MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::expected_range_v);
    static constexpr tick_type_strong range_low_v_ = calculate_lower_bound(
        MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::daily_close_v, MarketStats::expected_range_v);
    static constexpr tick_type_strong range_high_v_ = calculate_upper_bound(
        MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::daily_close_v, MarketStats::expected_range_v);

    // Hot path data: these get hammered on every order operation, so keep them together
    // for better cache locality. best_bid/ask are checked constantly, and the bitmaps
    // are used for efficient level scanning
    tick_type_strong best_bid_ = tick_type_strong::no_value();
    tick_type_strong best_ask_ = tick_type_strong::no_value();
    bitset_type bid_bitmap_{};
    bitset_type ask_bitmap_{};

    // Bulkier storage goes here - these are less frequently accessed as whole containers
    bid_storage bids_;
    ask_storage asks_;
    order_storage orders_;
    std::pmr::memory_resource* resource_;
};

} // namespace jazzy
