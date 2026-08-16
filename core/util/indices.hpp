//
// Created by akicatt on 08.08.2026.
//

#ifndef MYDAK_SERVER_INDICES_H
#define MYDAK_SERVER_INDICES_H
#include <cstddef>
#include <cstdint>
namespace mydak {
    struct client_index {
        client_index() = default;
        ~client_index() = default;
        client_index(const client_index& self) = default;

        static auto empty() {
            return client_index(invalid_index, invalid_index, invalid_index);
        }

        template <typename T, typename T1>
        client_index(const T index, const T1 generation)
        requires std::is_integral_v<T> && std::is_integral_v<T1>
        : index(static_cast<std::size_t>(index)), generation(static_cast<std::size_t>(generation)) {}

        template <typename T, typename T1, typename T2>
        client_index(const T index, const T1 generation, const T2 db_index)
        requires std::is_integral_v<T> && std::is_integral_v<T1> && std::is_integral_v<T2>
        : index(index), generation(generation), db_index(db_index) {}


        std::size_t index{};
        std::size_t generation{};
        std::uint64_t db_index{}; //TODO CHANGE TO INT64_T
        static constexpr std::size_t invalid_index = std::numeric_limits<std::size_t>::max();
    };
}
#endif //MYDAK_SERVER_INDICES_H
