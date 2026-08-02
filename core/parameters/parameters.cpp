//
// Created by akicatt on 01.08.2026.
//

#include <unordered_map>

#include "parameters.hpp"
#include "tools.hpp"

namespace parameters {
    static constexpr uint8_t max_uint8_t = std::numeric_limits<uint8_t>::max();
    static constexpr int8_t max_int8_t = std::numeric_limits<int8_t>::max();

    static const auto existing_arguments = mydak::tools::index_unordered_map(
        "--db-hostname",
        "--db-username",
        "--db-password"
    );

    static const std::vector<mydak::args::variants_wrapper> existing_parameters{
        mydak::args::parameter<2>("localhost"),
        mydak::args::parameter<1>(4, 64, "username"),
        mydak::args::parameter<1>(4, 64, "password")
    };
}

void mydak::args::help() {
    for (const auto& argument : parameters::existing_arguments) {
        const auto& variant_wrapper = parameters::existing_parameters[argument.second];
        variant_wrapper.variant.visit([argument](auto&& parameter) {
            logger::log(std::format("{} : {}", argument.first, parameter.limits_to_string()));
        });
    }

    std::exit(1);
}

[[nodiscard]] std::vector<mydak::args::variants_wrapper> mydak::args::process_args(const int argc, char* argv[]) {
    std::vector<variants_wrapper> values = parameters::existing_parameters;
    for (int i = 1; i < argc; i++) {
        std::string raw = argv[i];

        if (raw == "--help") help();

        const auto equals_pos = raw.find('=');
        if (equals_pos == std::string::npos) {
            logger::exception(std::format("Wrong parameter format: {}! (no equals symbol)", raw));
        }

        std::string parameter_string = raw.substr(0, equals_pos);
        auto value = std::string(raw.subview(equals_pos + 1, raw.size() - equals_pos - 1));

        if (value.empty()) {
            logger::exception(std::format("Empty value: {}!", parameter_string));
        }

        auto it = parameters::existing_arguments.find(parameter_string);

        if (it == parameters::existing_arguments.end()) {
            logger::exception(std::format("Wrong parameter: {}! seek help: --help.", parameter_string));
        }

        const size_t& index = it->second;
        auto& variant_wrapper = values[index];
        variant_wrapper.variant.visit([value](auto&& parameter) {
            parameter.try_set_val(value);
        });
    }

    return values;
}
