#pragma once

#include <asio/strand.hpp>
#include <asio/compose.hpp>

namespace atools {

template <typename Data, typename Executor>
class async_locked {
public:
    template <typename T>
    class proxy {
    public:
        proxy(T& data) : data_(data) {}

        constexpr T& operator*() const noexcept { return data_; }
        constexpr T* operator->() const noexcept { return &data_; }

    private:
        T& data_;
    };

    template <typename... Args>
    explicit async_locked(Executor executor, Args&&... args)
        : strand_(std::move(executor)), data_(std::forward<Args>(args)...)
    {
    }

    auto strand() const { return strand_; }

    template <typename CompletionToken>
    auto async_lock(CompletionToken token)
    {
        return asio::async_compose<CompletionToken, void(proxy<Data>)>(
            async_lock_impl<decltype(*this)>{*this}, token);
    }

    template <typename CompletionToken>
    auto async_lock(CompletionToken token) const
    {
        return asio::async_compose<CompletionToken, void(proxy<Data const>)>(
            async_lock_impl<decltype(*this)>{*this}, token);
    }

private:
    template <typename Parent>
    struct async_lock_impl {
        Parent& parent;

        template <typename Self>
        void operator()(Self& self)
        {
            asio::dispatch(parent.strand_, [this, self = std::move(self)]() mutable {
                self.complete(parent.data_);
            });
        }
    };

    asio::strand<Executor> strand_;
    Data data_;
};

} // namespace atools
