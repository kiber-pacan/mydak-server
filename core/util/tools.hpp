//
// Created by akicatt on 02.08.2026.
//

#ifndef MYDAK_SERVER_TOOLS_H
#define MYDAK_SERVER_TOOLS_H
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

namespace mydak::tools {
    template <typename T, typename... Chars>
    T index_map_template(Chars... names) {
        return [&names...] <std::size_t... Indices>(std::index_sequence<Indices...>) {
            return T{
                {std::string_view(names), std::integral_constant<std::size_t, Indices>{}}...
            };
        } (std::make_index_sequence<sizeof...(names)>());
    }



    template <typename... T>
    requires ((std::is_same_v<T, const char*>) && ...)
    constexpr auto index_unordered_map(T... names) {
        return index_map_template<std::unordered_map<std::string_view, std::size_t>>(names...);
    }

    template <typename... T>
    requires ((std::is_same_v<T, const char*>) && ...)
    auto index_map(T... names) {
        return index_map_template<std::map<std::string_view, std::size_t>>(names...);
    }

    template <std::size_t N>
    constexpr auto constexpr_index_array() {
        return [] <std::size_t... Indices>(std::index_sequence<Indices...>) {
            return std::array<std::size_t, N>{Indices...};
        } (std::make_index_sequence<N>());
    }

    // AT START
    template <std::size_t N, typename Sequence>
    struct at;

    template <std::size_t N, std::size_t... Indices>
    struct at<N, std::index_sequence<Indices...>> {
        // Creating array and getting from it by std::size_t by index, simple
        static constexpr std::size_t index = std::array<std::size_t, sizeof...(Indices)>{Indices...}[N];
    };

    template <std::size_t... Indices>
    constexpr std::size_t at_(std::size_t N, std::index_sequence<Indices...>) {
        return std::array<std::size_t, sizeof...(Indices)>{Indices...}[N];
    }
    // AT END

    // CONSTEXPR FOR START
    template <std::size_t N, typename F>
    constexpr void constexpr_for(F&& func) {
        [&] <std::size_t... Indices> (std::index_sequence<Indices...>) {
            (func(std::integral_constant<std::size_t, Indices>{}), ...);
        } (std::make_index_sequence<N>());
    }
    // CONSTEXPR FOR END

    // STATIC STRING START
    template<std::size_t N>
    struct static_string {
        constexpr static_string() = default;
        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr static_string(const char (&str)[N]) {
            std::copy_n(str, N, characters);
        }

        [[nodiscard]] constexpr const char* c_str() const {
            return characters;
        }

        constexpr bool operator == (std::string_view string_view) const {
            return std::string_view{characters, N - 1} == string_view;
        }

        constexpr bool operator < (std::string_view string_view) const {
            return std::string_view{characters, N - 1} < string_view;
        }

        constexpr bool operator > (std::string_view string_view) const {
            return std::string_view{characters, N - 1} > string_view;
        }

        char characters[N]{};
    };
    // STATIC STRING END


    inline struct
    {
        template <typename T>
        constexpr bool operator()(std::pair<std::string_view, T> a, std::pair<std::string_view, T> b) const {
            return a.first < b.first;
        }
    }
    compare1;

    inline struct
    {
        template <typename T>
        constexpr bool operator()(std::pair<std::string_view, T> a, const auto& b) const {
            return b > a.first;
        }
    }
    compare2;


    // STATIC MAP START
    template <std::size_t N, typename T>
    struct static_map {
        constexpr explicit static_map() = default;

        template <typename... Pairs>
        requires (std::is_same_v<Pairs, std::pair<const char*, T>> && ...)
        constexpr explicit static_map(Pairs... pairs)
            : array{std::make_pair(std::string_view{pairs.first}, pairs.second)...}
        {
            std::sort(array.begin(), array.end(), compare1);
        }

        template <static_string key>
        consteval T at() const {
            auto it = std::lower_bound(array.begin(), array.end(), key, compare2);
            if (it != array.end() && it->first == key) {
                return it->second;
            }

            throw std::out_of_range("Nonexisting key!");
        }

    private:
        std::array<std::pair<std::string_view, T>, N> array;
    };
    template <typename... Pairs>
    static_map(Pairs... pairs) -> static_map<sizeof...(Pairs), typename Pairs...[0]::second_type>;
    // STATIC MAP END

    template <typename... Chars>
    auto index_static_map(Chars... names) {
        return [&names...] <std::size_t... Indices>(std::index_sequence<Indices...>) {
            return static_map(
                std::make_pair(std::string_view(names), std::integral_constant<std::size_t, Indices>{})...
            );
        } (std::make_index_sequence<sizeof...(names)>());
    }
}
#endif //MYDAK_SERVER_TOOLS_H
