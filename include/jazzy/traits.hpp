#include <concepts>
#include <cstddef>

namespace jazzy {
namespace detail {

struct order_id_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_id_getter(obj);
    }
};

struct order_volume_getter
{
    template <typename T>
    constexpr auto operator()(T const& obj) const
    {
        return jazzy_order_volume_getter(obj);
    }
};

struct order_volume_setter
{
    template <typename T, typename U>
    constexpr void operator()(T& obj, U const& value) const
    {
        jazzy_order_volume_setter(obj, value);
    }
};

} // namespace detail

template <typename T>
using order_id_type = decltype(detail::order_id_getter{}(std::declval<T>()));

template <typename T>
using order_volume_type = decltype(detail::order_volume_getter{}(std::declval<T>()));

template <typename T>
concept order = requires(T const& order) {
    { detail::order_id_getter{}(order) } -> std::same_as<order_id_type<T>>;
    { detail::order_volume_getter{}(order) } -> std::same_as<order_volume_type<T>>;
    requires requires(order_volume_type<T> v1, order_volume_type<T> v2) {
        { v1 + v2 } -> std::same_as<order_volume_type<T>>;
    };
};

template <typename T>
concept tick = std::integral<T> && std::convertible_to<T, size_t>;

constexpr detail::order_id_getter order_id_getter;
constexpr detail::order_volume_getter order_volume_getter;
constexpr detail::order_volume_setter order_volume_setter;

} // namespace jazzy
