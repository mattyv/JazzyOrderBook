#pragma once

namespace jazzy::tests {
struct order
{
    int order_id{};
    int volume{};
};

inline int jazzy_order_id_getter(order const& o) { return o.order_id; }
inline int jazzy_order_volume_getter(order const& o) { return o.volume; }
inline void jazzy_order_volume_setter(order& o, int volume) { o.volume = volume; }

} // namespace jazzy::tests
