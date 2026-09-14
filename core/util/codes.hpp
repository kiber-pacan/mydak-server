//
// Created by akicatt on 17.08.2026.
//

#ifndef MYDAK_SERVER_CODES_H
#define MYDAK_SERVER_CODES_H
#include <cstddef>

namespace mydak {
    enum class send_codes {
        NO_CLIENT,
        EXPIRED_CLIENT,
        BAD_SIGNAL,
        SUCCESS,
        EXCEPTION
    };
    namespace send_strings {
        constexpr std::string_view NO_CLIENT =
            "No client with that index";
        constexpr std::string_view EXPIRED_CACHED_CLIENT =
            "Cached client is expired, getting new one!";
        constexpr std::string_view EXPIRED_CACHED_CLIENT_SECOND_TRY =
            "Cached client is still somehow expired after getting new one!";
        constexpr std::string_view BAD_SIGNAL =
            "Idk how you managed to fuck with signal channel.";
    }

}

#endif //MYDAK_SERVER_CODES_H
