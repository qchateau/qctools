// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <condition_variable>
#include <mutex>
#include <shared_mutex>

namespace qc {

namespace details {
template <typename Lock, typename ConstLock>
struct best_cv {
    using type = std::condition_variable_any;
};

template <>
struct best_cv<std::unique_lock<std::mutex>, std::unique_lock<std::mutex>> {
    using type = std::condition_variable;
};

template <typename Lock, typename ConstLock>
using best_cv_t = typename best_cv<Lock, ConstLock>::type;

template <typename F>
struct on_exit {
    F fct;
    ~on_exit() { fct(); }
};

template <typename F>
on_exit<std::decay_t<F>> call_on_exit(F&& fct)
{
    return {std::forward<F>(fct)};
}

} // namespace details

template <typename CRTP,
          typename T,
          typename Mutex = std::mutex,
          typename Lock = std::lock_guard<Mutex>,
          typename ConstLock = Lock>
class basic_tsafe {
private:
    mutable Mutex mutex_;
    T value_;

public:
    template <typename... Args>
    basic_tsafe(Args&&... args) : value_{std::forward<Args>(args)...}
    {
    }

    template <typename F, typename... LArgs>
    auto write_with_lock(F&& fct, LArgs&&... largs)
    {
        Lock lock{mutex_, std::forward<LArgs>(largs)...};
        return fct(value_, lock);
    }

    template <typename F, typename... LArgs>
    auto read_with_lock(F&& fct, LArgs&&... largs) const
    {
        ConstLock lock{mutex_, std::forward<LArgs>(largs)...};
        return fct(value_, lock);
    }

    template <typename F>
    auto write(F&& fct)
    {
        return static_cast<CRTP*>(this)->write_with_lock([&](auto& v, auto&) { return fct(v); });
    }

    template <typename F>
    auto read(F&& fct) const
    {
        return static_cast<const CRTP*>(this)->read_with_lock(
            [&](auto& v, auto&) { return fct(v); });
    }

    void swap(T& new_value)
    {
        return static_cast<CRTP*>(this)->write([&](auto& value) { std::swap(new_value, value); });
    }

    void set(T new_value)
    {
        return static_cast<CRTP*>(this)->write([&](auto& value) { value = std::move(new_value); });
    }

    T get() const
    {
        return static_cast<const CRTP*>(this)->read([&](auto& value) { return value; });
    }
};

template <typename T,
          typename Mutex = std::mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = Lock>
class tsafe : public basic_tsafe<tsafe<T, Mutex, Lock, ConstLock>, T, Mutex, Lock, ConstLock> {
public:
    using tsafe::basic_tsafe::basic_tsafe;
};

template <typename T,
          typename Mutex = std::mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = Lock>
using unique_tsafe = tsafe<T, Mutex, Lock, ConstLock>;

template <typename T,
          typename Mutex = std::timed_mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = Lock>
using timed_tsafe = tsafe<T, Mutex, Lock, ConstLock>;

template <typename T,
          typename Mutex = std::shared_mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = std::shared_lock<Mutex>>
using shared_tsafe = tsafe<T, Mutex, Lock, ConstLock>;

template <typename T,
          typename Mutex = std::shared_timed_mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = std::shared_lock<Mutex>>
using shared_timed_tsafe = tsafe<T, Mutex, Lock, ConstLock>;

template <typename CRTP,
          typename T,
          typename Mutex = std::mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = Lock,
          typename ConditionVariable = details::best_cv_t<Lock, ConstLock>>
class basic_waitable_tsafe : public basic_tsafe<CRTP, T, Mutex, Lock, ConstLock> {
private:
    using base = basic_tsafe<CRTP, T, Mutex, Lock, ConstLock>;

    mutable ConditionVariable cv_;

public:
    template <typename... Args>
    basic_waitable_tsafe(Args&&... args) : base{std::forward<Args>(args)...}
    {
    }

    template <typename F, typename... LArgs>
    auto write_with_lock(F&& fct, LArgs&&... largs)
    {
        auto exit = details::call_on_exit([this]() { cv_.notify_all(); });
        return static_cast<base*>(this)->write_with_lock(std::forward<F>(fct),
                                                         std::forward<LArgs>(largs)...);
    }

    template <typename F>
    auto wait(F&& fct) const
    {
        return static_cast<const CRTP*>(this)->read_with_lock(
            [&](auto& value, auto& lock) { return cv_.wait(lock, [&]() { return fct(value); }); });
    }

    template <typename Duration, typename F>
    auto wait_for(Duration&& duration, F&& fct) const
    {
        return static_cast<const CRTP*>(this)->read_with_lock([&](auto& value, auto& lock) {
            return cv_.wait_for(
                lock, std::forward<Duration>(duration), [&]() { return fct(value); });
        });
    }

    template <typename TimePoint, typename F>
    auto wait_until(TimePoint&& time_point, F&& fct) const
    {
        return static_cast<const CRTP*>(this)->read_with_lock([&](auto& value, auto& lock) {
            return cv_.wait_until(
                lock, std::forward<TimePoint>(time_point), [&]() { return fct(value); });
        });
    }

    void notify_one() { cv_.notify_one(); }
    void notify_all() { cv_.notify_all(); }
}; // namespace qc

template <typename T,
          typename Mutex = std::mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = Lock,
          typename ConditionVariable = details::best_cv_t<Lock, ConstLock>>
class waitable_tsafe
    : public basic_waitable_tsafe<waitable_tsafe<T, Mutex, Lock, ConstLock, ConditionVariable>,
                                  T,
                                  Mutex,
                                  Lock,
                                  ConstLock,
                                  ConditionVariable> {
public:
    using waitable_tsafe::basic_waitable_tsafe::basic_waitable_tsafe;
};

template <typename T,
          typename Mutex = std::timed_mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = Lock,
          typename ConditionVariable = details::best_cv_t<Lock, ConstLock>>
using timed_waitable_tsafe = waitable_tsafe<T, Mutex, Lock, ConstLock, ConditionVariable>;

template <typename T,
          typename Mutex = std::shared_mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = std::shared_lock<Mutex>,
          typename ConditionVariable = details::best_cv_t<Lock, ConstLock>>
using shared_waitable_tsafe = waitable_tsafe<T, Mutex, Lock, ConstLock, ConditionVariable>;

template <typename T,
          typename Mutex = std::shared_timed_mutex,
          typename Lock = std::unique_lock<Mutex>,
          typename ConstLock = std::shared_lock<Mutex>,
          typename ConditionVariable = details::best_cv_t<Lock, ConstLock>>
using shared_timed_waitable_tsafe = waitable_tsafe<T, Mutex, Lock, ConstLock, ConditionVariable>;

} // namespace qc
