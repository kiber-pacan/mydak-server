//
// Created by down on 13.08.2026.
//

#include "parameters_accessor.hpp"
#include "parameters.hpp"

template <mydak::tools::static_string Option>
consteval auto mydak::parameters_accessor::get_by_static_string() const {
    using type = std::decay_t<decltype(args::get_type_sequence())>;
    const auto N = args::options_indices.consteval_at<Option>();

    return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
}

template <std::size_t N>
consteval auto mydak::parameters_accessor::get_by_index(std::integral_constant<std::size_t, N>) {

}

template <std::size_t N>
constexpr mydak::parameters_accessor::parameters_accessor(std::array<args::parameter_variants, N> parameters_internal) {
    for (auto& variant : parameters_internal) {
        this->parameters_internal.emplace_back(&variant);
    }
}
