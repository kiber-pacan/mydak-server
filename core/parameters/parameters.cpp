//
// Created by akicatt on 01.08.2026.
//

#include <unordered_map>
#include <boost/asio/ip/address.hpp>
#include <boost/core/demangle.hpp>

#include "parameters.hpp"

bool mydak::args::is_a_number(const std::string_view text) {
    return !text.empty() ? std::ranges::all_of(text, [](auto& character){ return std::isdigit(static_cast<unsigned char>(character)); }) : false;
}

bool mydak::args::is_an_ip(const std::string& raw_ip) {
    if (raw_ip == "localhost") {
        logger::log_debug("localhost is discouraged from using!");
        return true;
    }
    boost::system::error_code error;
    boost::asio::ip::make_address(raw_ip, error);
    return error ? false : true;
}

void mydak::args::help() {
    for (const auto& argument : existing_arguments) {
        const auto& variant_wrapper = existing_parameters[argument.second];

        variant_wrapper.visit([argument](auto&& parameter) {
            logger::log(std::format("{} : {} ({})", argument.first, parameter.limits_to_string(), boost::core::demangle(typeid(parameter.get_data()).name())));
        });
    }

    std::exit(1);
}

[[nodiscard]] mydak::args::parameters_accessor mydak::args::process_args(const int argc, char* argv[]) {
    auto values = existing_parameters;
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

        auto it = existing_arguments.find(parameter_string);

        if (it == existing_arguments.end()) {
            logger::exception(std::format("Wrong parameter: {}! seek help: --help.", parameter_string));
        }

        const size_t& index = it->second;
        auto& variant_wrapper = values[index];
        variant_wrapper.visit([value](auto&& parameter) {
            parameter.try_set_val(value.c_str());
        });
    }

    return parameters_accessor(values);
}
