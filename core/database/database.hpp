//
// Created by akicatt on 01.08.2026.
//

#ifndef MYDAK_SERVER_DATABASE_H
#define MYDAK_SERVER_DATABASE_H

#include <iostream>
#include <boost/mysql.hpp>
#include <boost/asio.hpp>
#include <boost/charconv.hpp>

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

            // Connect to the server
            connection.connect(params);
        }

        void coro_main() {
            std::string create_request =
    std::format("CREATE TABLE IF NOT EXISTS users("
                "id INT AUTO_INCREMENT PRIMARY KEY,"
                "public_key VARCHAR({}));",
                proto::PUBLIC_KEY_L);

            mysql::results ignored_result;
            connection.execute(create_request, ignored_result);

            // Print the first field in the first row
            bool has_users_table = static_cast<bool>(ignored_result.rows().at(0).at(0).as_int64());
            std::cout << has_users_table << std::endl;



            // Close the connection
            connection.close();
        }


    private:
        asio::io_context& io;
        mysql::any_connection connection;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
