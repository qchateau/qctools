#include <asio/io_context.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>

#include "async_locked.h"

constexpr void check(bool value)
{
    if (!value) {
        throw std::runtime_error("bad check");
    }
}

class Foo {
public:
    explicit Foo(asio::io_context& io) : io_(io), data_(io.get_executor()) {}

    asio::awaitable<void> run()
    {
        check(!data_.strand().running_in_this_thread());
        {
            auto data = co_await data_.async_lock(asio::use_awaitable);
            static_assert(std::is_same_v<decltype(*data), Data&>);
        }
        check(data_.strand().running_in_this_thread());
        co_await asio::post(asio::use_awaitable);
        check(!data_.strand().running_in_this_thread());
        {
            auto data = co_await std::as_const(data_).async_lock(asio::use_awaitable);
            static_assert(std::is_same_v<decltype(*data), Data const&>);
        }
        check(data_.strand().running_in_this_thread());
    }

private:
    struct Data {
        int counter = 0;
    };
    asio::io_context& io_;
    atools::async_locked<Data, asio::io_context::executor_type> data_;
};

int main(int, char**)
{
    asio::io_context io;
    Foo foo(io);
    auto fut = asio::co_spawn(io, foo.run(), asio::use_future);
    io.run();
    fut.get();
    return 0;
}
