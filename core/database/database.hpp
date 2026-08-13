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

#include "proto.hpp"
#include "logger.hpp"

namespace mysql = boost::mysql;
namespace asio = boost::asio;


namespace mydak {
    struct database {
        explicit database(asio::io_context& io, std::string_view hostname, std::string_view username, std::string_view password) : io(io), connection(io) {
            try {
                // The hostname, username and password to use
                mysql::connect_params params;
                params.server_address.emplace_host_and_port(std::string(hostname));
                params.username = username;
                params.password = password;
                params.connection_collation = mysql::mariadb_collations::utf8mb4_general_ci;

                mysql::results result;

                // Connect to the server
                connection.connect(params);

                // Create database and then connect to it
                connection.execute("CREATE DATABASE IF NOT EXISTS mydak_database;", result);

                connection.execute("USE mydak_database;", result);
                auto create_request =
                    mysql::with_params(
                        "CREATE TABLE IF NOT EXISTS mydak_users("
                        "id INT AUTO_INCREMENT PRIMARY KEY,"
                        "public_key VARCHAR({}),"
                        "UNIQUE KEY public_key_unique (public_key),"
                        "messages JSON);",
                        proto::PUBLIC_KEY_L
                    );
                connection.execute(create_request, result);
            } catch (const std::exception& e) {
                logger::exception_func(e.what());
            }
        }



        asio::awaitable<std::uint64_t> add_user(const std::array<char, proto::PUBLIC_KEY_L>& public_key);

        std::uint64_t get_db_index(const std::array<char, proto::PUBLIC_KEY_L>& public_key);

        asio::awaitable<void> add_message(std::uint64_t index, const std::vector<char>& message);
    private:
        asio::io_context& io;
        mysql::any_connection connection;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
