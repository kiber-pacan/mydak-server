//
// Created by down on 21.08.2026.
//

#ifndef MYDAK_BACKEND_COH_H
#define MYDAK_BACKEND_COH_H
#include <boost/asio.hpp>
namespace asio = boost::asio;

// coh - coroutine helper
namespace mydak::coh {
    inline auto& io() {
        static asio::io_context io{};
        return io;
    }

    template <typename T>
    void detached(asio::awaitable<T>&& coroutine_call)
    requires std::is_rvalue_reference_v<decltype(coroutine_call)>
    {
        asio::co_spawn(io(), std::move(coroutine_call), asio::detached);
    }

    template <typename T>
    struct is_awaitable : std::false_type {};

    template <typename T>
    struct is_awaitable<asio::awaitable<T>> : std::true_type {};

    template <typename Func>
    void detached(Func&& coroutine_lambda)
    requires
    std::is_rvalue_reference_v<decltype(coroutine_lambda)> &&
    std::is_invocable_v<Func> &&
    is_awaitable<std::invoke_result_t<Func>>::value
    {
        asio::co_spawn(io(), coroutine_lambda, asio::detached);
    }
} // mydak::coh

#endif //MYDAK_BACKEND_COH_H
