#pragma once

#include <cassert>
#include <concepts>
#include <optional>
#include <utility>

namespace jazzy::detail {

template <typename IdType>
struct intrusive_fifo_node
{
    std::optional<IdType> prev{};
    std::optional<IdType> next{};
    bool in_queue{false};

    void reset() noexcept
    {
        prev.reset();
        next.reset();
        in_queue = false;
    }
};

template <typename Lookup, typename IdType>
concept intrusive_fifo_lookup =
    requires(Lookup&& lookup, IdType id) {
        { std::forward<Lookup>(lookup)(id) } -> std::same_as<intrusive_fifo_node<IdType>&>;
    };

template <typename IdType>
class intrusive_fifo_queue
{
public:
    using id_type = IdType;
    using node_type = intrusive_fifo_node<id_type>;

    [[nodiscard]] bool empty() const noexcept { return !head_.has_value(); }
    [[nodiscard]] std::optional<id_type> front() const noexcept { return head_; }
    [[nodiscard]] std::optional<id_type> back() const noexcept { return tail_; }

    template <typename Lookup>
        requires intrusive_fifo_lookup<Lookup, id_type>
    void push_back(const id_type& id, Lookup&& lookup)
    {
        auto& node = lookup(id);
        assert(!node.in_queue && "Node already enqueued");

        node.prev = tail_;
        node.next.reset();
        node.in_queue = true;

        if (tail_)
        {
            auto& tail_node = lookup(*tail_);
            tail_node.next = id;
        }
        else
        {
            head_ = id;
        }

        tail_ = id;
    }

    template <typename Lookup>
        requires intrusive_fifo_lookup<Lookup, id_type>
    void erase(const id_type& id, Lookup&& lookup)
    {
        auto& node = lookup(id);
        assert(node.in_queue && "Node not enqueued");

        auto prev = node.prev;
        auto next = node.next;

        if (prev)
        {
            auto& prev_node = lookup(*prev);
            prev_node.next = next;
        }
        else
        {
            head_ = next;
        }

        if (next)
        {
            auto& next_node = lookup(*next);
            next_node.prev = prev;
        }
        else
        {
            tail_ = prev;
        }

        node.reset();
    }

    template <typename Lookup>
        requires intrusive_fifo_lookup<Lookup, id_type>
    void move_to_back(const id_type& id, Lookup&& lookup)
    {
        auto& node = lookup(id);
        if (!node.in_queue || !node.next)
            return;

        erase(id, lookup);
        push_back(id, lookup);
    }

    template <typename Lookup>
        requires intrusive_fifo_lookup<Lookup, id_type>
    void clear(Lookup&& lookup)
    {
        auto current = head_;
        while (current)
        {
            auto& node = lookup(*current);
            auto next = node.next;
            node.reset();
            current = next;
        }
        reset();
    }

    void reset() noexcept
    {
        head_.reset();
        tail_.reset();
    }

private:
    std::optional<id_type> head_{};
    std::optional<id_type> tail_{};
};

} // namespace jazzy::detail
