#define CATCH_CONFIG_MAIN
#include <future>
#include <list>
#include <shared_mutex>

#include "tsafe.hpp"
#include <catch2/catch.hpp>

using namespace qc;

class non_movable {
public:
    explicit non_movable(int i, double d) : i_{i}, d_{d} {}
    non_movable(const non_movable&) = delete;
    non_movable(non_movable&&) = delete;
    non_movable& operator=(const non_movable&) = delete;
    non_movable& operator=(non_movable&&) = delete;

    bool operator==(const non_movable& other) const { return i_ == other.i_ && d_ == other.d_; }

private:
    const int i_;
    const double d_;
};

class my_mutex : private std::shared_timed_mutex {
public:
    using std::shared_timed_mutex::shared_timed_mutex;

    using std::shared_timed_mutex::lock;
    using std::shared_timed_mutex::lock_shared;
    using std::shared_timed_mutex::try_lock;
    using std::shared_timed_mutex::try_lock_for;
    using std::shared_timed_mutex::try_lock_shared;
    using std::shared_timed_mutex::try_lock_shared_for;
    using std::shared_timed_mutex::unlock;
    using std::shared_timed_mutex::unlock_shared;
};

class my_unique_lock : private std::unique_lock<my_mutex> {
public:
    using std::unique_lock<my_mutex>::unique_lock;

    using std::unique_lock<my_mutex>::lock;
    using std::unique_lock<my_mutex>::unlock;
    using std::unique_lock<my_mutex>::owns_lock;
};

class my_shared_lock : private std::shared_lock<my_mutex> {
public:
    using std::shared_lock<my_mutex>::shared_lock;

    using std::shared_lock<my_mutex>::lock;
    using std::shared_lock<my_mutex>::unlock;
    using std::shared_lock<my_mutex>::owns_lock;
};

template <typename TSafe>
std::future<void> wait_value(TSafe& safe, int value)
{
    std::list<std::future<void>> futures;

    futures.emplace_back(std::async(std::launch::async, [&safe, value] {
        safe.wait([value](auto& v) { return v == value; });
    }));
    futures.emplace_back(std::async(std::launch::async, [&safe, value] {
        safe.wait_for(std::chrono::seconds{3600}, [value](auto& v) { return v == value; });
    }));
    futures.emplace_back(std::async(std::launch::async, [&safe, value] {
        safe.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds{3600},
                        [value](auto& v) { return v == value; });
    }));

    return std::async(std::launch::async, [futures = std::move(futures)]() mutable {
        for (auto& f : futures) {
            f.get();
        }
    });
};

bool done_soon(std::future<void>&& future)
{
    return future.wait_for(std::chrono::seconds{1}) == std::future_status::ready;
}

template <typename T>
using TSafeBasic = std::tuple<tsafe<T>,
                              unique_tsafe<T>,
                              timed_tsafe<T>,
                              shared_tsafe<T>,
                              shared_timed_tsafe<T>,
                              tsafe<T, my_mutex, my_unique_lock, my_shared_lock>,
                              waitable_tsafe<T>,
                              timed_waitable_tsafe<T>,
                              shared_waitable_tsafe<T>,
                              shared_timed_waitable_tsafe<T>,
                              waitable_tsafe<T, my_mutex, my_unique_lock, my_shared_lock>>;

TEMPLATE_LIST_TEST_CASE("tsafe basic functions", "[tsafe][template]", TSafeBasic<int>)
{
    TestType safe{1};

    REQUIRE(safe.get() == 1);
    REQUIRE(safe.read([](auto& v) { return v; }) == 1);
    REQUIRE(safe.write([](auto& v) { return v; }) == 1);

    SECTION("write changes the value")
    {
        safe.write([](auto& v) { v = 42; });
        REQUIRE(safe.get() == 42);
    }

    SECTION("set changes the value")
    {
        safe.set(42);
        REQUIRE(safe.get() == 42);
    }

    SECTION("swap changes both the value and the other variable")
    {
        int other = 12;
        safe.swap(other);

        REQUIRE(safe.get() == 12);
        REQUIRE(other == 1);
    }

    SECTION("const is usable")
    {
        const auto& const_safe{safe};
        REQUIRE(const_safe.get() == 1);
        REQUIRE(const_safe.read([](auto& v) { return v; }) == 1);
    }
}

TEMPLATE_LIST_TEST_CASE("tsafe basic functions with non movable type",
                        "[tsafe][non_movable][template]",
                        TSafeBasic<non_movable>)
{
    TestType safe{12, 0.3};
    REQUIRE(safe.read([&](auto& v) { return v == non_movable{12, 0.3}; }));
}

template <typename T>
using TSafeUnique = std::tuple<unique_tsafe<T>,
                               timed_tsafe<T>,
                               shared_tsafe<T>,
                               shared_timed_tsafe<T>,
                               tsafe<T, my_mutex, my_unique_lock, my_shared_lock>,
                               waitable_tsafe<T>,
                               timed_waitable_tsafe<T>,
                               shared_waitable_tsafe<T>,
                               shared_timed_waitable_tsafe<T>,
                               waitable_tsafe<T, my_mutex, my_unique_lock, my_shared_lock>>;

TEMPLATE_LIST_TEST_CASE("tsafe unique functions", "[unique_tsafe][template]", TSafeUnique<int>)
{
    TestType safe{12};

    SECTION("lock with std::defer_lock")
    {
        REQUIRE(safe.read_with_lock([](auto&, auto& lock) { return !lock.owns_lock(); },
                                    std::defer_lock));
        REQUIRE(safe.write_with_lock([](auto&, auto& lock) { return !lock.owns_lock(); },
                                     std::defer_lock));
    }

    SECTION("lock with std::try_to_lock when already locked")
    {
        auto stolen_lock = safe.read_with_lock([](auto&, auto& lock) { return std::move(lock); });

        REQUIRE(stolen_lock.owns_lock());

        auto stolen_lock2 =
            std::async(std::launch::async, [&] {
                return safe.write_with_lock([](auto&, auto& lock) { return std::move(lock); },
                                            std::try_to_lock);
            }).get();

        REQUIRE(!stolen_lock2.owns_lock());
    }
}

template <typename T>
using TSafeTimed = std::tuple<timed_tsafe<T>,
                              shared_timed_tsafe<T>,
                              tsafe<T, my_mutex, my_unique_lock, my_shared_lock>,
                              timed_waitable_tsafe<T>,
                              shared_timed_waitable_tsafe<T>,
                              waitable_tsafe<T, my_mutex, my_unique_lock, my_shared_lock>>;

TEMPLATE_LIST_TEST_CASE("tsafe timed functions", "[timed_tsafe][template]", TSafeTimed<int>)
{
    TestType safe{12};

    SECTION("lock with a timeout")
    {
        auto stolen_lock = safe.read_with_lock([&](auto&, auto& lock) { return std::move(lock); },
                                               std::chrono::nanoseconds{1});

        REQUIRE(stolen_lock.owns_lock());

        auto stolen_lock2 =
            std::async(std::launch::async, [&] {
                return safe.write_with_lock([](auto&, auto& lock) { return std::move(lock); },
                                            std::chrono::nanoseconds{1});
            }).get();

        REQUIRE(!stolen_lock2.owns_lock());
    }
}

template <typename T>
using TSafeShared = std::tuple<shared_tsafe<T>,
                               shared_timed_tsafe<T>,
                               tsafe<T, my_mutex, my_unique_lock, my_shared_lock>,
                               shared_waitable_tsafe<T>,
                               shared_timed_waitable_tsafe<T>,
                               waitable_tsafe<T, my_mutex, my_unique_lock, my_shared_lock>>;

TEMPLATE_LIST_TEST_CASE("tsafe shared functions", "[shared_tsafe][template]", TSafeShared<int>)
{
    TestType safe{12};
    const TestType& const_safe{safe};

    SECTION("multiple shared lock")
    {
        auto stolen_shared_lock = const_safe.read_with_lock(
            [](auto&, auto& lock) { return std::move(lock); }, std::try_to_lock);

        auto stolen_shared_lock2 =
            std::async(std::launch::async, [&] {
                return safe.read_with_lock([](auto&, auto& lock) { return std::move(lock); },
                                           std::try_to_lock);
            }).get();

        REQUIRE(stolen_shared_lock.owns_lock());
        REQUIRE(stolen_shared_lock2.owns_lock());

        auto stolen_exclusive_lock =
            std::async(std::launch::async, [&] {
                return safe.write_with_lock([](auto&, auto& lock) { return std::move(lock); },
                                            std::try_to_lock);
            }).get();

        REQUIRE(!stolen_exclusive_lock.owns_lock());
    }
}

template <typename T>
using TSafeSharedTimed = std::tuple<shared_timed_tsafe<T>,
                                    tsafe<T, my_mutex, my_unique_lock, my_shared_lock>,
                                    shared_timed_waitable_tsafe<T>,
                                    waitable_tsafe<T, my_mutex, my_unique_lock, my_shared_lock>>;

TEMPLATE_LIST_TEST_CASE("tsafe shared functions", "[shared_tsafe][template]", TSafeSharedTimed<int>)
{
    TestType safe{12};
    const TestType& const_safe{safe};

    SECTION("multiple shared lock")
    {
        auto stolen_shared_lock = const_safe.read_with_lock(
            [](auto&, auto& lock) { return std::move(lock); }, std::chrono::nanoseconds{1});

        auto stolen_shared_lock2 =
            std::async(std::launch::async, [&] {
                return safe.read_with_lock([](auto&, auto& lock) { return std::move(lock); },
                                           std::chrono::nanoseconds{1});
            }).get();

        REQUIRE(stolen_shared_lock.owns_lock());
        REQUIRE(stolen_shared_lock2.owns_lock());

        auto stolen_exclusive_lock =
            std::async(std::launch::async, [&] {
                return safe.write_with_lock([](auto&, auto& lock) { return std::move(lock); },
                                            std::chrono::nanoseconds{1});
            }).get();

        REQUIRE(!stolen_exclusive_lock.owns_lock());
    }
}

template <typename T>
using WaitableTSafe = std::tuple<waitable_tsafe<T>,
                                 timed_waitable_tsafe<T>,
                                 shared_waitable_tsafe<T>,
                                 shared_timed_waitable_tsafe<T>,
                                 waitable_tsafe<T, my_mutex, my_unique_lock, my_shared_lock>>;

TEMPLATE_LIST_TEST_CASE("waitable tsafe", "[waitable_tsafe][template]", WaitableTSafe<int>)
{
    TestType safe{12};
    const TestType& const_safe{safe};

    SECTION("wait already true") { REQUIRE(done_soon(wait_value(safe, 12))); }
    SECTION("wait already true") { REQUIRE(done_soon(wait_value(const_safe, 12))); }

    SECTION("wait and set")
    {
        auto future = wait_value(safe, 42);
        safe.set(42);
        REQUIRE(done_soon(std::move(future)));
    }

    SECTION("const wait and set")
    {
        auto future = wait_value(const_safe, 42);
        safe.set(42);
        REQUIRE(done_soon(std::move(future)));
    }

    SECTION("wait and swap")
    {
        int expected = 42;
        auto future = wait_value(safe, expected);
        safe.swap(expected);
        REQUIRE(done_soon(std::move(future)));
    }

    SECTION("wait and write")
    {
        auto future = wait_value(safe, 42);
        safe.write([](auto& v) { v = 42; });
        REQUIRE(done_soon(std::move(future)));
    }

    SECTION("wait and write_with_lock")
    {
        auto future = wait_value(safe, 42);
        safe.write_with_lock([](auto& v, auto&) { v = 42; });
        REQUIRE(done_soon(std::move(future)));
    }
}
