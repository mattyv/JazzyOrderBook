#include <catch.hpp>

#include <jazzy/detail/intrusive_fifo.hpp>

#include <unordered_map>

TEST_CASE("intrusive FIFO queue maintains links", "[intrusive_fifo]")
{
    using queue_type = jazzy::detail::intrusive_fifo_queue<int>;
    using node_type = jazzy::detail::intrusive_fifo_node<int>;

    queue_type queue;
    std::unordered_map<int, node_type> nodes;
    auto lookup = [&](int id) -> node_type& {
        return nodes.at(id);
    };

    SECTION("push, move, and erase update adjacency")
    {
        nodes.emplace(1, node_type{});
        nodes.emplace(2, node_type{});
        nodes.emplace(3, node_type{});

        queue.push_back(1, lookup);
        REQUIRE(queue.front().has_value());
        REQUIRE(queue.front().value() == 1);
        REQUIRE(queue.back().value() == 1);
        REQUIRE(nodes.at(1).in_queue);
        REQUIRE_FALSE(nodes.at(1).prev.has_value());
        REQUIRE_FALSE(nodes.at(1).next.has_value());

        queue.push_back(2, lookup);
        REQUIRE(queue.front().value() == 1);
        REQUIRE(queue.back().value() == 2);
        REQUIRE(nodes.at(1).next.has_value());
        REQUIRE(nodes.at(1).next.value() == 2);
        REQUIRE(nodes.at(2).prev.has_value());
        REQUIRE(nodes.at(2).prev.value() == 1);

        queue.push_back(3, lookup);
        REQUIRE(queue.front().value() == 1);
        REQUIRE(queue.back().value() == 3);
        REQUIRE(nodes.at(2).next.has_value());
        REQUIRE(nodes.at(2).next.value() == 3);
        REQUIRE(nodes.at(3).prev.has_value());
        REQUIRE(nodes.at(3).prev.value() == 2);

        queue.move_to_back(1, lookup);
        REQUIRE(queue.front().value() == 2);
        REQUIRE(queue.back().value() == 1);
        REQUIRE(nodes.at(2).prev == std::nullopt);
        REQUIRE(nodes.at(2).next.has_value());
        REQUIRE(nodes.at(2).next.value() == 3);
        REQUIRE(nodes.at(3).next.has_value());
        REQUIRE(nodes.at(3).next.value() == 1);
        REQUIRE(nodes.at(1).prev.has_value());
        REQUIRE(nodes.at(1).prev.value() == 3);
        REQUIRE_FALSE(nodes.at(1).next.has_value());

        queue.erase(3, lookup);
        REQUIRE(queue.front().value() == 2);
        REQUIRE(queue.back().value() == 1);
        REQUIRE(nodes.at(3).in_queue == false);
        REQUIRE(nodes.at(2).next.has_value());
        REQUIRE(nodes.at(2).next.value() == 1);
        REQUIRE(nodes.at(1).prev.has_value());
        REQUIRE(nodes.at(1).prev.value() == 2);

        queue.erase(2, lookup);
        REQUIRE(queue.front().value() == 1);
        REQUIRE(queue.back().value() == 1);
        REQUIRE(nodes.at(1).prev == std::nullopt);
        REQUIRE(nodes.at(1).next == std::nullopt);

        queue.erase(1, lookup);
        REQUIRE(queue.empty());
    }

    SECTION("clear releases all nodes")
    {
        nodes.emplace(10, node_type{});
        nodes.emplace(11, node_type{});
        nodes.emplace(12, node_type{});

        queue.push_back(10, lookup);
        queue.push_back(11, lookup);
        queue.push_back(12, lookup);

        queue.clear(lookup);
        REQUIRE(queue.empty());
        REQUIRE_FALSE(queue.front().has_value());
        REQUIRE_FALSE(queue.back().has_value());

        for (auto id : {10, 11, 12})
        {
            INFO("Node id " << id);
            const auto& node = nodes.at(id);
            REQUIRE(node.in_queue == false);
            REQUIRE_FALSE(node.prev.has_value());
            REQUIRE_FALSE(node.next.has_value());
        }
    }
}
