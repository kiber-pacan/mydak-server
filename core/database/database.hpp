//
// Created by akicatt on 01.08.2026.
//

#ifndef MYDAK_SERVER_DATABASE_H
#define MYDAK_SERVER_DATABASE_H

#include <iostream>
#include <queue>
#include <boost/mysql.hpp>
#include <boost/asio.hpp>
#include <boost/charconv.hpp>
#include "boost/asio/experimental/channel.hpp"
#include <nlohmann/json.hpp>

#include "logger.hpp"
#include "proto.hpp"

namespace mysql = boost::mysql;
namespace asio = boost::asio;

// TODO FIX MULTIPLE EXECUTABLES IN WITHOUT CHECKING FOR STILL HANGING ONES
// TODO FIX MULTIPLE EXECUTABLES IN WITHOUT CHECKING FOR STILL HANGING ONES
// TODO FIX MULTIPLE EXECUTABLES IN WITHOUT CHECKING FOR STILL HANGING ONES
// TODO FIX MULTIPLE EXECUTABLES IN WITHOUT CHECKING FOR STILL HANGING ONES
// TODO FIX MULTIPLE EXECUTABLES IN WITHOUT CHECKING FOR STILL HANGING ONES
namespace mydak {
    struct db_message {
        db_message() = default;
        db_message(const std::vector<char>& data, std::size_t db_index)
        : data(data), db_index(db_index) {}


        std::vector<char> data{};
        std::size_t db_index{};
    };

    struct database {
        using lock_channel = asio::experimental::channel<void(boost::system::error_code)>;

        explicit database(asio::io_context& io, std::string_view hostname, std::string_view username, std::string_view password);

        asio::awaitable<std::uint64_t> add_user(const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& public_key);

        asio::awaitable<void> add_message(std::uint64_t index, const std::vector<char>& message);

        asio::awaitable<std::vector<db_message>> get_delayed_messages(std::uint64_t db_index);

        asio::awaitable<std::uint64_t> get_db_index(const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& public_key);

        asio::awaitable<void> delete_delayed_messages(std::vector<std::uint64_t> db_indices);

        template <BOOST_MYSQL_EXECUTION_REQUEST T>
        asio::awaitable<void> async_execute(
            T request,
            mysql::results& result
        );

        template <BOOST_MYSQL_EXECUTION_REQUEST T>
        void execute(
            T request,
            mysql::results& result
        );

        lock_channel& add_request();
    private:
        asio::io_context& io;
        mysql::any_connection connection;

        bool first_request = true;
        std::deque<lock_channel> lock_channels;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
