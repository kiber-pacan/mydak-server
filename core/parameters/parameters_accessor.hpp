//
// Created by down on 13.08.2026.
//

#ifndef MYDAK_SERVER_PARAMETERS_ACCESSOR_H
#define MYDAK_SERVER_PARAMETERS_ACCESSOR_H
#include "tools.hpp"
#include <vector>

#include "parameters.hpp"


namespace mydak {
    struct parameters_accessor {
        constexpr ~parameters_accessor() = default;
        constexpr parameters_accessor() = default;
        template <std::size_t N>
        constexpr parameters_accessor(const std::array<args::parameter_variants, N>& parameters_internal) : parameters_internal(parameters_internal) {}

        template <std::size_t N>
        constexpr auto get() const {
            // Getting type from our magic constexpr type_indices
            using type = std::decay_t<decltype(args::type_sequence)>;

            return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
        }

        template <tools::static_string Option>
        constexpr auto get() const {
            using type = std::decay_t<decltype(args::get_type_sequence())>;
            const auto N = args::options_indices.consteval_at<Option>();

            return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
        }
    private:
        std::vector<args::parameter_variants*> parameters_internal;
    };
}


#endif //MYDAK_SERVER_PARAMETERS_ACCESSOR_H
