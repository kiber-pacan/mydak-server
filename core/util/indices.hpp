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
            return client_index(-1, -1);
        }

        client_index(const std::size_t index, const std::uint64_t db_index)
        : index(index), db_index(db_index) {}

        client_index(const std::size_t index)
        : index(index), db_index(-1) {}


        std::size_t index;
        std::uint64_t db_index;
    };

    struct recipient_index {
        recipient_index() = default;
        ~recipient_index() = default;
        recipient_index(const recipient_index& self) = default;

        static auto empty() {
            return recipient_index(-1, -1, -1);
        }

        recipient_index(const client_index& index, const std::size_t generation) {
            this->index = index.index;
            this->generation = generation;
            this->db_index = index.db_index;
        }
        recipient_index(const std::size_t index, const std::size_t generation, const std::uint64_t db_index)
        : index(index), generation(generation), db_index(db_index) {}


        std::size_t index{};
        std::size_t generation{};
        std::uint64_t db_index{}; //TODO CHANGE TO INT64_T
    };
}
#endif //MYDAK_SERVER_INDICES_H
