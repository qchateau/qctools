#pragma once

#include <utility>

namespace qctools {

template <typename S, typename T, std::size_t... I>
S to_struct(T&& tup, std::index_sequence<I...>)
{
    return S{std::get<I>(std::forward<T>(tup))...};
}

template <typename S, typename T>
S to_struct(T&& tup)
{
    return to_struct<S>(std::forward<T>(tup),
                        std::make_index_sequence<std::tuple_size<std::decay_t<T>>{}>{});
}
} // namespace qctools
