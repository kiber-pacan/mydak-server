//
// Created by akicatt on 01.08.2026.
//

#ifndef MYDAK_SERVER_DATABASE_H
#define MYDAK_SERVER_DATABASE_H

#include <iostream>
#include <boost/mysql.hpp>
#include <boost/asio.hpp>
#include <boost/charconv.hpp>
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

            // Connect to the server
            connection.connect(params);
        }

        asio::awaitable<void> coro_main() {
            const char* request = "SELECT 'Hello world!'";
            mysql::results result;
            co_await connection.async_execute(request, result);

            // Print the first field in the first row
            std::cout << result.rows().at(0).at(0) << std::endl;

            // Close the connection
            co_await connection.async_close();
        }


    private:
        asio::io_context& io;
        mysql::any_connection connection;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
