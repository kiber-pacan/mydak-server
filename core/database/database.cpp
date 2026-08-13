//
// Created by akicatt on 01.08.2026.
//

#include "database.hpp"


asio::awaitable<std::uint64_t> mydak::database::add_user(const std::array<char, proto::PUBLIC_KEY_L>& public_key) {
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
        logger::exception_func(e.what());
    }
    co_return std::uint64_t{};
}


asio::awaitable<void> mydak::database::add_message(std::uint64_t index, const std::vector<char>& message) {
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
        logger::log_debug_error(e.what());
    }

    co_return;
}


std::uint64_t mydak::database::get_db_index(const std::array<char, proto::PUBLIC_KEY_L>& public_key) {
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
        logger::log_debug_error(e.what());
        return 0;
    }
}