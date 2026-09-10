//
// Created by akicatt on 17.08.2026.
//

#ifndef MYDAK_SERVER_CODES_H
#define MYDAK_SERVER_CODES_H
#include <cstddef>

namespace mydak::codes {
    constexpr std::uint8_t NO_CLIENT = 0;
    constexpr std::uint8_t EXPIRED_CLIENT = 1;
    constexpr std::uint8_t BAD_SIGNAL = 2;
    constexpr std::uint8_t SUCCESS = 3;
    constexpr std::uint8_t EXCEPTION = -1;
}

#endif //MYDAK_SERVER_CODES_H
