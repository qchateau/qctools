// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <tuple>
#include <utility>

namespace qctools {
namespace details {
template <typename...>
struct logic_and;

template <typename B, typename... Bs>
struct logic_and<B, Bs...> {
    static constexpr bool value = B::value && logic_and<Bs...>::value;
};

template <typename B>
struct logic_and<B> {
    static constexpr bool value = B::value;
};

template <typename T0, typename T1>
auto min(T0 v0, T1 v1)
{
    return std::min(v0, v1);
}
template <typename T, typename... Ts>
auto min(T v, Ts... vs)
{
    return std::min(v, min(vs...));
}

template <typename... T>
void do_nothing(T&&...){};

template <typename... C>
class zip_impl {
    using seq = std::make_index_sequence<sizeof...(C)>;
    static constexpr bool is_const = logic_and<std::is_const<std::remove_reference_t<C>>...>::value;

public:
    template <typename... Iterators>
    class iterator_impl {
    public:
        explicit iterator_impl(std::tuple<Iterators...> iterators) : iterators(iterators) {}

        auto operator*() { return deref_impl(seq{}); }
        iterator_impl& operator++() { return inc_impl(seq{}); }
        bool operator==(const iterator_impl& other) const { return iterators == other.iterators; }
        bool operator!=(const iterator_impl& other) const { return iterators != other.iterators; }

    private:
        template <std::size_t... I>
        iterator_impl& inc_impl(std::index_sequence<I...>)
        {
            do_nothing(std::get<I>(iterators).operator++()...);
            return *this;
        }

        template <std::size_t... I>
        auto deref_impl(std::index_sequence<I...>)
        {
            return std::forward_as_tuple((*std::get<I>(iterators))...);
        }

        std::tuple<Iterators...> iterators;
    };

    using iterator = iterator_impl<typename std::decay_t<C>::iterator...>;
    using const_iterator = iterator_impl<typename std::decay_t<C>::const_iterator...>;

    explicit zip_impl(C&&... containers) : containers(std::forward<C>(containers)...) {}

    auto begin() { return begin_impl(seq{}); }
    auto end() { return end_impl(seq{}); }

private:
    template <std::size_t... I>
    auto begin_impl(std::index_sequence<I...>)
    {
        return std::conditional_t<is_const, const_iterator, iterator>{
            std::make_tuple(std::get<I>(containers).begin()...)};
    }

    template <std::size_t... I>
    auto end_impl(std::index_sequence<I...>)
    {
        auto distance =
            min(std::distance(std::get<I>(containers).begin(), std::get<I>(containers).end())...);
        return std::conditional_t<is_const, const_iterator, iterator>{
            std::make_tuple(std::next(std::get<I>(containers).begin(), distance)...)};
    }

    std::tuple<C&&...> containers;
};

} // namespace details

template <class... Containers>
auto zip(Containers&&... containers)
{
    return details::zip_impl<Containers&&...>{std::forward<Containers>(containers)...};
}
} // namespace qctools
