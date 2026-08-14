//
// Created by akicatt on 01.08.2026.
//

#include <unordered_map>
#include <boost/asio/ip/address.hpp>
#include <boost/core/demangle.hpp>

#include "parameters.hpp"



// Output order is undefined
void mydak::args::help() {
    /*
    for (const auto& argument : existing_arguments) {
        const auto& variant_wrapper = existing_parameters[argument.second];

        variant_wrapper.visit([argument](auto&& parameter) {
            logger::log(std::format("{} : {} ({})", argument.first, parameter.limits_to_string(), boost::core::demangle(typeid(decltype(parameter.get_data())).name())));
        });
    }*/
    std::array<std::string_view, std::size(parameters)> array{};
    tools::constexpr_for<std::size(parameters)>(
        [&](auto i) {
            array[i] = std::string_view{std::get<i>(options_tuple).c_str()};
        }
    );

    for (std::size_t i = 0; i < std::size(parameters); i++) {
        parameters[i].visit([&](auto&& parameter) {
        logger::log(std::format("{} : {} ({}), default value: {}.", array[i], parameter.limits_to_string(), boost::core::demangle(typeid(decltype(parameter.get_data())).name()), parameter.get_data()));
        });
    }

    std::exit(1);
}

[[nodiscard]] mydak::parameters_accessor mydak::args::process_args(const int argc, char* argv[]) {
    auto new_parameters = parameters;
    immortal_strings.resize(128);

    for (int i = 1; i < argc; i++) {
        std::string raw = argv[i];

        if (raw == "--help") help();

        const auto equals_pos = raw.find('=');
        if (equals_pos == std::string::npos) {
            logger::exception(std::format("Wrong parameter format: {}! (no equals symbol)", raw));
        }

        std::string parameter_string = raw.substr(0, equals_pos);
        immortal_strings[i] = std::string(raw.subview(equals_pos + 1, raw.size() - equals_pos - 1));
        const auto& value = immortal_strings[i];

        if (value.empty()) {
            logger::exception(std::format("Empty value: {}!", parameter_string));
        }

        auto opt = options_indices.at(parameter_string);

        if (!opt.has_value()) {
            logger::exception(std::format("Wrong parameter: {}! seek help: --help.", parameter_string));
        }

        const size_t& index = opt.value();
        parameter_variants& variants = new_parameters[index];
        variants.visit([&](auto&& parameter) {
            parameter.try_set_val(value.c_str());
        });
    }

    return {new_parameters};
}
