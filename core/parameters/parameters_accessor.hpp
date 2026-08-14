//
// Created by akicatt on 14.08.2026.
//

#ifndef MYDAK_SERVER_PARAMETERS_ACCESSOR_H
#define MYDAK_SERVER_PARAMETERS_ACCESSOR_H

#include "parameters.hpp"

namespace mydak::args {
    struct parameters_accessor {
        constexpr ~parameters_accessor() = default;
        constexpr parameters_accessor() = default;
        parameters_accessor(const int argc, char* argv[]) : parameters_internal(process_args(argc, argv)) {}

        template <std::size_t N>
        constexpr auto get() const {
            // Getting type from our magic constexpr type_indices
            using type = std::decay_t<decltype(type_sequence)>;

            return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
        }

        template <tools::static_string Option>
        constexpr auto get() const {
            using type = std::decay_t<decltype(get_type_sequence())>;
            const auto N = options_indices.consteval_at<Option>();

            return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
        }
    private:
        [[nodiscard]] static std::array<parameter_variants, parameters_count> process_args( int argc, char* argv[]);

        std::array<parameter_variants, parameters_count> parameters_internal;
    };
}




#endif //MYDAK_SERVER_PARAMETERS_ACCESSOR_H
