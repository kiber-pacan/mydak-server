//
// Created by akicatt on 14.08.2026.
//

#include "parameters_accessor.hpp"

[[nodiscard]] std::array<mydak::args::parameter_variants, mydak::args::parameters_count> mydak::args::parameters_accessor::process_args( int argc, char* argv[]) {
    auto new_parameters = parameters;
    immortal_strings.resize(128);

    for (int i = 1; i < argc; i++) {
        std::string raw = argv[i];

        if (raw == "--help") help();

        const auto equals_pos = raw.find('=');
        if (equals_pos == std::string::npos) {
            logger::exit(std::format("Wrong parameter format: {}! (no equals symbol)", raw));
        }

        std::string parameter_string = raw.substr(0, equals_pos);
        immortal_strings[i] = std::string(raw.subview(equals_pos + 1, raw.size() - equals_pos - 1));
        const auto& value = immortal_strings[i];

        if (value.empty()) {
            logger::exit(std::format("Empty value: {}!", parameter_string));
        }

        auto opt = options_indices.at(parameter_string);

        if (!opt.has_value()) {
            logger::exit(std::format("Wrong parameter: {}! seek help: --help.", parameter_string));
        }

        const size_t& index = opt.value();
        parameter_variants& variants = new_parameters[index];
        variants.visit([&](auto&& parameter) {
            parameter.try_set_val(value.c_str());
        });
    }

    return new_parameters;
}