//
// Created by akicatt on 01.08.2026.
//

#include "database.hpp"



mydak::database::database(asio::io_context& io, std::string_view hostname, std::string_view username, std::string_view password) : io(io), connection(io) {
    try {
        // The hostname, username and password to use
        mysql::connect_params params;
        params.server_address.emplace_host_and_port(std::string(hostname));
        params.username = username;
        params.password = password;
        //params.connection_collation = mysql::mariadb_collations::utf8mb4_general_ci;

        mysql::results result;

        // Connect to the server
        connection.connect(params);

        // Create database and then use it
        connection.execute("CREATE DATABASE IF NOT EXISTS mydak_database;", result);
        connection.execute("USE mydak_database;", result);

        auto users_request =
            mysql::with_params(
                "CREATE TABLE IF NOT EXISTS mydak_users("
                    "id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
                    "public_key VARCHAR({}),"
                    // Prevents adding new user with public key that already exists in table
                    "UNIQUE KEY public_key_unique (public_key)"
                ");",
                proto::E2E_KEYS_L
            );
        connection.execute(users_request, result);

        auto messages_request =
            mysql::with_params(
                "CREATE TABLE IF NOT EXISTS mydak_messages("
                    "id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
                    // Relative id from mydak_users table
                    "user_id BIGINT UNSIGNED NOT NULL,"
                    // Message blob (binary stuff)
                    "data LONGBLOB NOT NULL,"
                    // Making so that mydak_messages references mydak_users
                    // Also when deleting user his messages get deleted!!!
                    "FOREIGN KEY (user_id) REFERENCES mydak_users(id) ON DELETE CASCADE"
                ");"
            );
        connection.execute(messages_request, result);
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
}



asio::awaitable<std::uint64_t> mydak::database::add_user(const std::array<char, proto::E2E_KEYS_L>& public_key) {
    try {
        mysql::results result;

        co_await connection.async_execute(
            mysql::with_params(
                "INSERT IGNORE INTO mydak_users (public_key) VALUES ({})",
                std::string_view(public_key.data(), public_key.size())
            ),
            result,
            asio::use_awaitable
        );

        if (result.affected_rows() > 0) {
            co_return result.last_insert_id();
        } else {
            co_await connection.async_execute(
                mysql::with_params(
                    "SELECT id FROM mydak_users WHERE public_key = {}",
                    std::string_view(public_key.data(), public_key.size())
                ),
                result,
                asio::use_awaitable
            );
            co_return result.rows().at(0).at(0).as_uint64();
        }
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
    co_return -1;
}

asio::awaitable<void> mydak::database::add_message(std::uint64_t index, const std::vector<char>& message) {
    try {
        const auto raw_data = reinterpret_cast<const unsigned char*>(message.data());
        mysql::blob_view message_blob(raw_data, message.size());

        mysql::results result;
        auto request =
        mysql::with_params(
            "INSERT INTO mydak_messages (user_id, data) VALUES ({}, {});",
            index,
            message_blob
        );

        co_await connection.async_execute(
            request,
            result,
            asio::use_awaitable
        );

        co_await connection.async_execute(
            "SELECT id, user_id, data FROM mydak_messages;",
            result,
            asio::use_awaitable
        );


        for (const auto& row : result.rows()) {
            const auto& id = row.at(0);
            const auto& user_id = row.at(1);
            const auto& data = row.at(2);
            logger::log_debug(std::format("{} {} {}", id.as_uint64(), user_id.as_uint64(), data.as_blob()));
        }
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }

    co_return;
}



asio::awaitable<std::vector<mydak::db_message>> mydak::database::get_delayed_messages(std::uint64_t db_index) {
    mysql::results result;
    try {
        co_await connection.async_execute(
            mysql::with_params(
                // Getting messages by index with ascending order
                "SELECT data, id FROM mydak_messages WHERE user_id = {} ORDER BY id ASC;",
                db_index
            ),
            result,
            asio::use_awaitable
        );

        std::vector<db_message> messages{};
        const auto& rows = result.rows();

        if (rows.empty()) co_return messages;
        messages.reserve(rows.size());


        for (const auto& row : rows) {
            messages.emplace_back(std::vector<char>(row.at(0).as_blob().begin(), row.at(0).as_blob().end()), row.at(1).as_uint64());
        }

        co_return messages;
    } catch (const std::exception& e) {
        logger::log_debug_error(result.has_value() ? result.info() : e.what());
    }

    // Empty vector
    co_return std::vector<db_message>{};
}

std::uint64_t mydak::database::get_db_index(const std::array<char, proto::E2E_KEYS_L>& public_key) {
    try {
        mysql::results result;
        connection.execute(
            mysql::with_params(
                "SELECT id FROM mydak_users WHERE public_key = {}",
                std::string_view(public_key.data(), public_key.size())
            ),
            result
        );

        return result.rows().at(0).at(0).as_uint64();
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
    return std::uint64_t{};
}

asio::awaitable<void> mydak::database::delete_delayed_messages(const std::vector<std::uint64_t> db_indices) {
    if (db_indices.empty()) co_return;
    try {
        mysql::results result;
        co_await connection.async_execute(
            mysql::with_params(
                "DELETE from mydak_messages WHERE id IN ({})",
                mysql::sequence(db_indices, [](const std::uint64_t& id, mysql::format_context_base& ctx) {
                    ctx.append_value(id);
                })
            ),
            result,
            asio::use_awaitable
        );
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
}
