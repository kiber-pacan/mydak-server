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

    namespace detail {
        template <typename T>
        struct is_awaitable : std::false_type {};

        template <typename T>
        struct is_awaitable<asio::awaitable<T>> : std::true_type {};

        template <typename T>
        concept co_lambda =
            std::is_invocable_v<T> &&
            is_awaitable<std::invoke_result_t<T>>::value;
    }



    template <typename T>
    void detached(asio::awaitable<T>&& coroutine_call)
    requires std::is_rvalue_reference_v<decltype(coroutine_call)>
    {
        asio::co_spawn(io(), std::move(coroutine_call), asio::detached);
    }

    template <typename Func>
    void detached(Func&& coroutine_lambda)
    {
        asio::co_spawn(io(), std::forward<Func>(coroutine_lambda), asio::detached);
    }


    template <typename T>
    auto future(asio::awaitable<T>&& coroutine_call)
    requires std::is_rvalue_reference_v<decltype(coroutine_call)>
    {
        return asio::co_spawn(io(), std::move(coroutine_call), asio::use_future);
    }

    template <typename Func>
    auto future(Func&& coroutine_lambda)
    requires detail::co_lambda<Func>
    {
        return asio::co_spawn(io(), std::forward<Func>(coroutine_lambda), asio::use_future);
    }
} // mydak::coh

#endif //MYDAK_BACKEND_COH_H
