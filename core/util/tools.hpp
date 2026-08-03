//
// Created by akicatt on 02.08.2026.
//

#ifndef MYDAK_SERVER_TOOLS_H
#define MYDAK_SERVER_TOOLS_H
#include <map>
#include <string>
#include <unordered_map>

namespace mydak::tools {
    template <typename T, typename... Chars>
    requires std::is_same_v<T, std::map<std::string_view, std::size_t>> ||
             std::is_same_v<T, std::unordered_map<std::string_view, std::size_t>>
    T index_map_template(Chars... names) {
        return [&names...] <std::size_t... Indices>(std::index_sequence<Indices...>) {
            return T{
                {std::string_view(names), Indices}...
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
    auto constexpr_indexed_array() {
        return [] <std::size_t... Indices>(std::index_sequence<Indices...>) {
            return std::array<std::size_t, N>{Indices...};
        } (std::make_index_sequence<N>());
    }
}
#endif //MYDAK_SERVER_TOOLS_H
