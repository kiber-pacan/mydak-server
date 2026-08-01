//
// Created by akicatt on 01.08.2026.
//

#ifndef MYDAK_SERVER_DATABASE_H
#define MYDAK_SERVER_DATABASE_H

#include <boost/mysql.hpp>
namespace mysql = boost::mysql;
namespace asio = boost::asio;


namespace mydak {
    struct database {
        explicit database(asio::io_context& io, const std::string& hostname, const std::string& username, const std::string& password) : io(io) {

            mysql::any_connection connection(io);

            // The hostname, username and password to use
            mysql::connect_params params;
            params.server_address.emplace_host_and_port(hostname);
            params.username = username;
            params.password = password;

            // Connect to the server
            connection.connect(params);

        }

    private:
        asio::io_context& io;
    };
}


#endif //MYDAK_SERVER_DATABASE_H
