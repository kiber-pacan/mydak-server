//
// Created by akicatt on 01.08.2026.
//

#include <unordered_map>
#include <boost/asio/ip/address.hpp>
#include <boost/core/demangle.hpp>

#include "parameters.hpp"



// Output order is undefined
void mydak::args::help() {
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
