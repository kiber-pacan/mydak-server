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

namespace mysql = boost::mysql;
namespace asio = boost::asio;


namespace mydak {
    struct database {
        explicit database(asio::io_context& io, std::string_view hostname, std::string_view username, std::string_view password) : io(io), connection(io) {
            // The hostname, username and password to use
            mysql::connect_params params;
            params.server_address.emplace_host_and_port(std::string(hostname));
            params.username = username;
            params.password = password;
            //params.connection_collation = mysql::mariadb_collations::utf8mb4_general_ci;

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
        }



        asio::awaitable<std::uint64_t> add_user(const std::array<char, proto::PUBLIC_KEY_L>& public_key) {
            try {
                nlohmann::json user;
                user["messages"] = nlohmann::json::array();

                mysql::results result;

                co_await connection.async_execute(
                    mysql::with_params(
                        "INSERT IGNORE INTO mydak_users (public_key, messages) VALUES ({}, {})",
                        std::string_view(public_key.data(), public_key.size()),
                        user.dump()
                    ),
                    result,
                    asio::use_awaitable
                );



                co_await connection.async_execute(
                    "SELECT id, public_key, messages FROM mydak_users;",
                    result,
                    asio::use_awaitable
                );

                std::uint64_t index = result.rows().at(0).at(0).as_int64();

                for (const auto& row : result.rows()) {
                    const auto& id = row.at(0);
                    const auto& key = row.at(1);
                    const auto& messages = row.at(2);
                    std::cout << std::format("{} {} {}", id.as_int64(), key.as_string(), messages.as_string()) << std::endl;
                }

                co_return index;
            } catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
            }
            co_return std::uint64_t{};
        }

        asio::awaitable<void> add_message(std::uint64_t index, const std::vector<char>& message) {
            try {
                mysql::results result;

                auto request =
                mysql::with_params(
                    "UPDATE mydak_users"
                    "SET messages = JSON_ARRAY_APPEND(messages, '$.messages', {})"
                    "WHERE id = {};",
                    std::string_view(message.data(), message.size()),
                    index
                );

                co_await connection.async_execute(
                    request,
                    result,
                    asio::use_awaitable
                );

                co_await connection.async_execute(
                    "SELECT id, public_key, messages FROM mydak_users;",
                    result,
                    asio::use_awaitable
                );

                for (const auto& row : result.rows()) {
                    const auto& id = row.at(0);
                    const auto& key = row.at(1);
                    const auto& messages = row.at(2);
                    std::cout << std::format("{} {} {}", id.as_int64(), key.as_string(), messages.as_string()) << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
            }

            co_return;
        }

        std::uint64_t get_db_index(const std::array<char, proto::PUBLIC_KEY_L>& public_key) {
            try {
                mysql::results result;
                connection.execute(
                    mysql::with_params(
                        "SELECT id FROM mydak_users WHERE public_key = {}",
                        std::string_view(public_key.data(), public_key.size())
                    ),
                    result
                );

                return result.rows().at(0).at(0).as_int64();
            } catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
                return 0;
            }
        }

    private:
        asio::io_context& io;
        mysql::any_connection connection;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
