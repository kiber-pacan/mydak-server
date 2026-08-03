#ifndef MYDAK_WEBSOCKET_CORE_MAIN_CPP
#define MYDAK_WEBSOCKET_CORE_MAIN_CPP


#include <boost/asio/detail/chrono.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <sched.h>
#include <source_location>
#include <tuple>
#include <utility>

#include <boost/asio.hpp>

#include "main/server.hpp"
#include "logger.hpp"

#include "parameters/parameters.hpp"
#include "database.hpp"

namespace asio = boost::asio;

constexpr std::string_view SERVER_SHUT_DOWN =
	"The server shut down!";
constexpr std::string_view SERVER_STARTED =
	"The server started!";










int main(const int argc, char* argv[]) {
	asio::io_context io;
	const auto parameters = mydak::args::process_args(argc, argv);

	std::cout << parameters.get<0>() << std::endl;
	std::cout << parameters.get<1>() << std::endl;
	std::cout << parameters.get<2>() << std::endl;


	//mydak::database database(io, std::get<2>(parameters[0]).get_data(), std::get<2>(parameters[1]).get_data(), std::get<2>(parameters[2]).get_data());

	/*
	try {
		const auto server = std::make_shared<mydak::server>(io);
		server.get()->start_accepting_connections();
		mydak::logger::log_debug(SERVER_STARTED);
		io.run();
	}
	catch (std::exception& e) {
		mydak::logger::log_debug_error(e.what());
	}

	mydak::logger::log_debug(SERVER_SHUT_DOWN);
	*/
	return 0;
}

#endif  // MYDAK_WEBSOCKET_CORE_MAIN_CPP
