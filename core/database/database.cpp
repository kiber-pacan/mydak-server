//
// Created by akicatt on 01.08.2026.
//

#include "database.hpp"

#include "coh.hpp"

#pragma region Initialization
mydak::database::database(asio::io_context& io)
    : io(io), connection(io),
      // Setting capacity to zero so we won't get any expired signals
      // about getting new request when we in a loop already
      // tackling them
      request_add_channel(io, 0)
{}

void mydak::database::start(std::string_view hostname, std::string_view username, std::string_view password) {
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
                    "public_key BINARY(32) NOT NULL,"
                    // Prevents adding new user with public key that already exists in table
                    "UNIQUE KEY public_key_unique (public_key)"
                ");",
                proto::E2E_KEYS_RAW_L
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

        // Create detached async execution loop
        coh::detached(async_execute_queued_loop());
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
}
#pragma endregion

#pragma region Add
asio::awaitable<std::uint64_t> mydak::database::add_user(const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& public_key) {
    try {
        mysql::results result;
        mysql::blob_view public_key_blob(public_key.data(), std::size(public_key));


        co_await async_execute_queued(
            mysql::format_sql(
                connection.format_opts().value(),
                "INSERT IGNORE INTO mydak_users (public_key) VALUES ({})",
                public_key_blob
            ),
            result
        );


        if (result.affected_rows() > 0) {
            co_return result.last_insert_id();
        } else {
            co_await async_execute_queued(
                mysql::format_sql(
                    connection.format_opts().value(),
                    "SELECT id FROM mydak_users WHERE public_key = {}",
                    public_key_blob
                ),
                result
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
        mysql::format_sql(
            connection.format_opts().value(),
            "INSERT INTO mydak_messages (user_id, data) VALUES ({}, {});",
            index,
            message_blob
        );

        co_await async_execute_queued(
            request,
            result
        );

        /*co_await async_execute(
            "SELECT id, user_id, data FROM mydak_messages;",
            result,
            asio::use_awaitable
        );


        for (const auto& row : result.rows()) {
            const auto& id = row.at(0);
            const auto& user_id = row.at(1);
            const auto& data = row.at(2);
            logger::log_debug(std::format("{} {} {}", id.as_uint64(), user_id.as_uint64(), data.as_blob()));
        }*/
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }

    co_return;
}
#pragma endregion

#pragma region Delayed
asio::awaitable<std::vector<mydak::db_message>> mydak::database::get_delayed_messages(std::uint64_t db_index) {
    mysql::results result;
    try {
        co_await async_execute_queued(
            mysql::format_sql(
                connection.format_opts().value(),
                // Getting messages by index with ascending order
                "SELECT data, id FROM mydak_messages WHERE user_id = {} ORDER BY id ASC;",
                db_index
            ),
            result
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

asio::awaitable<void> mydak::database::delete_delayed_messages(const std::vector<std::uint64_t> db_indices) {
    if (db_indices.empty()) co_return;
    try {

        mysql::results result;
        co_await async_execute_queued(
            mysql::format_sql(
                connection.format_opts().value(),
                "DELETE from mydak_messages WHERE id IN ({})",
                mysql::sequence(db_indices, [](const std::uint64_t& id, mysql::format_context_base& ctx) {
                    ctx.append_value(id);
                })
            ),
            result
        );
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
}
#pragma endregion

#pragma region Helper
asio::awaitable<std::uint64_t> mydak::database::get_db_index(const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& public_key) {
    try {
        mysql::blob_view public_key_blob(public_key.data(), std::size(public_key));

        mysql::results result;
        connection.execute(
            mysql::with_params(
                "SELECT id FROM mydak_users WHERE public_key = {}",
                public_key_blob
            ),
            result
        );

        co_return result.rows().at(0).at(0).as_uint64();
    } catch (const std::exception& e) {
        logger::log_func_debug_error(e.what());
    }
    co_return std::uint64_t{};
}
#pragma endregion

#pragma region Async queue
/**
 * Helper method for async wait until we have something in the deque
 */
asio::awaitable<void> mydak::database::wait_until_non_empty() {
    if (lock_channels.empty()) {
        co_await request_add_channel.async_receive();
    }
    co_return;
}

/**
 * Helper method for adding request to the channel and pinging loop
 */
auto& mydak::database::add_request(
    std::string_view request,
    mysql::results& result
) {
    auto& added_channel = lock_channels.emplace_front(channel(io.get_executor()), request, result);

    // Ping our async_execute_queued_loop
    boost::system::error_code ec;
    request_add_channel.try_send(ec);

    return added_channel;
}

/**
 * Helper method for calling requests
 * that send to the mariadb
 */
asio::awaitable<void> mydak::database::async_execute_queued(
    const std::string_view request,
    mysql::results& result
) {
    // std::tuple<channel, std::string, mysql::results&>
    // getting channel and waiting until we get signal that means
    // we got result
    co_await get<0>(add_request(request, result)).async_receive();

    co_return;
}

/**
 * We have separate loop bcs when we
 * have multiple clients we often
 * get concurrent async access database
 * and we cant have that, so we implemented
 * loop that executes requests and sends
 * signal back to the caller of async_execute() func
 */
asio::awaitable<void> mydak::database::async_execute_queued_loop() {
    try {
        for (;;) {
            co_await wait_until_non_empty();
            auto& [channel, request, result] = lock_channels.back();

            // Request
            co_await connection.async_execute(
                request,
                result
            );

            // Signal
            boost::system::error_code ec;
            co_await channel.async_send(ec);

            // Clear
            lock_channels.pop_back();
        }
    } catch (const std::exception& e) {
        logger::log_error(e.what());
    }
}
#pragma endregion