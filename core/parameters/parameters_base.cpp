//
// Created by down on 14.08.2026.
//

#include "parameters_base.hpp"
#include <boost/asio.hpp>
#include <boost/system.hpp>


namespace asio = boost::asio;

// Negative numbers is not gonna pass this test
bool mydak::args::is_a_number(const std::string_view text) {
    return !text.empty() ? std::ranges::all_of(text, [](auto& character){ return std::isdigit(static_cast<unsigned char>(character)); }) : false;
}

bool mydak::args::is_an_ip(std::string_view raw_ip) {
    if (raw_ip == "localhost") {
        logger::log_debug("localhost is discouraged from using!");
        return true;
    }
    boost::system::error_code error;
    asio::ip::make_address(raw_ip, error);
    return error ? false : true;
}

