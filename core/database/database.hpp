//
// Created by akicatt on 01.08.2026.
//

#ifndef MYDAK_SERVER_DATABASE_H
#define MYDAK_SERVER_DATABASE_H

#include <iostream>
#include <boost/mysql.hpp>
#include <boost/asio.hpp>
#include <boost/charconv.hpp>
#include <nlohmann/json.hpp>

#include "logger.hpp"
#include "proto.hpp"

namespace mysql = boost::mysql;
namespace asio = boost::asio;


namespace mydak {
    struct database {
        explicit database(asio::io_context& io, std::string_view hostname, std::string_view username, std::string_view password);

        asio::awaitable<std::uint64_t> add_user(const std::array<char, proto::PUBLIC_KEY_L>& public_key);

        asio::awaitable<void> add_message(std::uint64_t index, const std::vector<char>& message);

        asio::awaitable<std::vector<std::vector<char>>> get_delayed_messages(std::uint64_t db_index);

        std::uint64_t get_db_index(const std::array<char, proto::PUBLIC_KEY_L>& public_key);

    private:
        asio::io_context& io;
        mysql::any_connection connection;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
