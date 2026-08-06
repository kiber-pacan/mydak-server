#ifndef MYDAK_SERVER_CORE_MAIN_CPP
#define MYDAK_SERVER_CORE_MAIN_CPP


#include <boost/asio/detail/chrono.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <sched.h>
#include <source_location>
#include <tuple>
#include <utility>

#include <boost/asio.hpp>
#include <boost/core/demangle.hpp>

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

	mydak::database database(io, parameters.get<"--db-hostname">(), parameters.get<"--db-username">(), parameters.get<"--db-password">());
	/*asio::co_spawn(
		io,
		database.coro_main(),
		asio::detached
	);*/

	//database.coro_main();

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

	return 0;
}

#endif  // MYDAK_SERVER_CORE_MAIN_CPP