#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <jazzy/traits.hpp>
#include <jazzy/types.hpp>

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <jazzy/detail/level_bitmap.hpp>

namespace jazzy {

template <typename TickType, typename OrderType, typename MarketStats, typename Allocator = std::allocator<OrderType>>
requires tick<TickType> && order<OrderType> && market_stats<MarketStats> &&
    std::same_as<typename MarketStats::tick_type, TickType> &&
    std::same_as<typename std::allocator_traits<Allocator>::value_type, OrderType> && compatible_allocator<Allocator>
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

        // Default constructor
        level() = default;

        // Allocator-extended default constructor
        // If volume_type uses allocators, construct it with the allocator; otherwise ignore
        template <typename Alloc>
        explicit level(std::allocator_arg_t, const Alloc& alloc)
            : volume(std::make_obj_using_allocator<volume_type>(alloc))
        {}

        // Allocator-extended copy constructor
        template <typename Alloc>
        level(std::allocator_arg_t, const Alloc& alloc, const level& other)
            : volume(std::make_obj_using_allocator<volume_type>(alloc, other.volume))
        {}

        // Allocator-extended move constructor
        template <typename Alloc>
        level(std::allocator_arg_t, const Alloc& alloc, level&& other)
            : volume(std::make_obj_using_allocator<volume_type>(alloc, std::move(other.volume)))
        {}
    };

public:
    using value_type = OrderType;
    using size_type = size_t;
    using allocator_type = Allocator;

private:
    using tick_type_strong = typename MarketStats::tick_type_strong_t;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using order_map_value_type = std::pair<const id_type, order_type>;
    using level_allocator_type = typename allocator_traits::template rebind_alloc<level>;
    using order_storage_allocator_type = typename allocator_traits::template rebind_alloc<order_map_value_type>;

    using propagate_on_container_copy_assignment = typename allocator_traits::propagate_on_container_copy_assignment;
    using propagate_on_container_move_assignment = typename allocator_traits::propagate_on_container_move_assignment;

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

    using bid_storage = std::vector<level, level_allocator_type>;
    using ask_storage = std::vector<level, level_allocator_type>;
    using order_storage = std::
        unordered_map<id_type, order_type, std::hash<id_type>, std::equal_to<id_type>, order_storage_allocator_type>;

public:
    using bitset_type = detail::level_bitmap<calculate_size(
        MarketStats::daily_high_v, MarketStats::daily_low_v, MarketStats::expected_range_v)>;

    order_book()
        : order_book(allocator_type{})
    {}

    explicit order_book(const allocator_type& allocator)
        : bids_(level_allocator_type(allocator))
        , asks_(level_allocator_type(allocator))
        , orders_(order_storage_allocator_type(allocator))
        , allocator_(allocator)
    {
        bids_.resize(size_);
        asks_.resize(size_);
        orders_.reserve(size_ * 10); // Arbitrary factor to reduce rehashing
    }

    ~order_book() = default;

    order_book(const order_book& other)
        : best_bid_(other.best_bid_)
        , best_ask_(other.best_ask_)
        , bid_bitmap_(other.bid_bitmap_)
        , ask_bitmap_(other.ask_bitmap_)
        , bids_(
              other.bids_,
              level_allocator_type(allocator_traits::select_on_container_copy_construction(other.allocator_)))
        , asks_(
              other.asks_,
              level_allocator_type(allocator_traits::select_on_container_copy_construction(other.allocator_)))
        , orders_(
              other.orders_,
              order_storage_allocator_type(allocator_traits::select_on_container_copy_construction(other.allocator_)))
        , allocator_(allocator_traits::select_on_container_copy_construction(other.allocator_))
    {}

    order_book(order_book&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
        : best_bid_(std::move(other.best_bid_))
        , best_ask_(std::move(other.best_ask_))
        , bid_bitmap_(std::move(other.bid_bitmap_))
        , ask_bitmap_(std::move(other.ask_bitmap_))
        , bids_(std::move(other.bids_))
        , asks_(std::move(other.asks_))
        , orders_(std::move(other.orders_))
        , allocator_(std::move(other.allocator_))
    {}

    order_book& operator=(const order_book& other)
    {
        if (this != &other)
        {
            // Create a copy, then swap with it (copy-and-swap idiom)
            order_book temp(other);

            using std::swap;

            // Handle allocator propagation
            if constexpr (propagate_on_container_copy_assignment::value)
            {
                swap(allocator_, temp.allocator_);
            }

            swap(best_bid_, temp.best_bid_);
            swap(best_ask_, temp.best_ask_);
            swap(bid_bitmap_, temp.bid_bitmap_);
            swap(ask_bitmap_, temp.ask_bitmap_);
            swap(bids_, temp.bids_);
            swap(asks_, temp.asks_);
            swap(orders_, temp.orders_);
        }
        return *this;
    }

    order_book& operator=(order_book&& other) noexcept(
        propagate_on_container_move_assignment::value || allocator_traits::is_always_equal::value)
    {
        if (this != &other)
        {
            if constexpr (propagate_on_container_move_assignment::value)
            {
                // Propagate allocator and move everything
                allocator_ = std::move(other.allocator_);
                bids_ = std::move(other.bids_);
                asks_ = std::move(other.asks_);
                orders_ = std::move(other.orders_);
                best_bid_ = std::move(other.best_bid_);
                best_ask_ = std::move(other.best_ask_);
                bid_bitmap_ = std::move(other.bid_bitmap_);
                ask_bitmap_ = std::move(other.ask_bitmap_);
            }
            else
            {
                // Check if allocators are equal
                if (allocator_ == other.allocator_)
                {
                    // Same allocator, can move efficiently
                    bids_ = std::move(other.bids_);
                    asks_ = std::move(other.asks_);
                    orders_ = std::move(other.orders_);
                    best_bid_ = std::move(other.best_bid_);
                    best_ask_ = std::move(other.best_ask_);
                    bid_bitmap_ = std::move(other.bid_bitmap_);
                    ask_bitmap_ = std::move(other.ask_bitmap_);
                }
                else
                {
                    // Different allocators, must copy everything to keep source valid
                    bids_ = other.bids_;
                    asks_ = other.asks_;
                    orders_ = other.orders_;
                    best_bid_ = other.best_bid_;
                    best_ask_ = other.best_ask_;
                    bid_bitmap_ = other.bid_bitmap_;
                    ask_bitmap_ = other.ask_bitmap_;
                }
            }
        }
        return *this;
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

        assert(succ && "Order ID already exists");

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

        assert(succ && "Order ID already exists");

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

        // Early exit for no-op updates
        if (tick_strong == original_tick && supplied_volume == original_volume)
            return;

        order_volume_setter(it->second, supplied_volume);
        order_tick_setter(it->second, tick_strong.value());

        if (tick_strong == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            size_type index = tick_to_index(original_tick);
            bids_[index].volume += volume_delta;
            const bool has_volume = bids_[index].volume != 0;
            update_bitsets<true>(has_volume, index);

            // Only scan if we might have removed the best bid
            if (volume_delta < 0 && original_tick == best_bid_ && !has_volume)
            {
                best_bid_ = scan_for_best_bid();
            }
        }
        else
        {
            size_type old_index = tick_to_index(original_tick);
            bids_[old_index].volume -= original_volume;
            update_bitsets<true>(bids_[old_index].volume != 0, old_index);

            size_type new_index = tick_to_index(tick_strong);
            bids_[new_index].volume += supplied_volume;
            update_bitsets<true>(true, new_index);

            // Update best_bid if needed
            if (tick_strong > best_bid_)
            {
                best_bid_ = tick_strong;
            }
            else if (original_tick == best_bid_ && bids_[old_index].volume == 0)
            {
                best_bid_ = scan_for_best_bid();
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

        // Early exit for no-op updates
        if (tick_strong == original_tick && supplied_volume == original_volume)
            return;

        order_volume_setter(it->second, supplied_volume);
        order_tick_setter(it->second, tick_strong.value());

        if (tick_strong == original_tick)
        {
            auto volume_delta = supplied_volume - original_volume;
            size_type index = tick_to_index(original_tick);
            asks_[index].volume += volume_delta;
            const bool has_volume = asks_[index].volume != 0;
            update_bitsets<false>(has_volume, index);

            // Only scan if we might have removed the best ask
            if (volume_delta < 0 && original_tick == best_ask_ && !has_volume)
            {
                best_ask_ = scan_for_best_ask();
            }
        }
        else
        {
            size_type old_index = tick_to_index(original_tick);
            asks_[old_index].volume -= original_volume;
            update_bitsets<false>(asks_[old_index].volume != 0, old_index);

            size_type new_index = tick_to_index(tick_strong);
            asks_[new_index].volume += supplied_volume;
            update_bitsets<false>(true, new_index);

            // Update best_ask if needed
            if (tick_strong < best_ask_)
            {
                best_ask_ = tick_strong;
            }
            else if (original_tick == best_ask_ && asks_[old_index].volume == 0)
            {
                best_ask_ = scan_for_best_ask();
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
                best_bid_ = scan_for_best_bid();
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
                best_ask_ = scan_for_best_ask();
            }
        }
    }

    order_type get_order(id_type id) const
    {
        auto it = orders_.find(id);
        if (it != orders_.end())
        {
            return it->second;
        }
        throw std::out_of_range("Order ID not found");
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
        const size_type total_levels = static_cast<size_type>(bid_bitmap_.count());
        if (level >= total_levels)
        {
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
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
        if (!best_ask_.has_value())
        {
            // No asks exist, return empty order
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
        }

        const size_type total_levels = static_cast<size_type>(ask_bitmap_.count());
        if (level >= total_levels)
        {
            order_type dummy_order{};
            order_volume_setter(dummy_order, 0);
            order_tick_setter(dummy_order, tick_type{});
            return dummy_order;
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

    [[nodiscard]] allocator_type get_allocator() const noexcept { return allocator_; }

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
        if (index < 0)
            return tick_type_strong::no_value();
        return index_to_tick(static_cast<size_type>(index));
    }

    tick_type_strong scan_for_best_ask() const
    {
        const int index = ask_bitmap_.find_lowest();
        if (index < 0)
            return tick_type_strong::no_value();
        return index_to_tick(static_cast<size_type>(index));
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
    [[no_unique_address]] allocator_type allocator_;
};

} // namespace jazzy
