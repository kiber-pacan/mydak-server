#ifndef MYDAK_WEBSOCKET_CORE_MAIN_CPP
#define MYDAK_WEBSOCKET_CORE_MAIN_CPP


#include <boost/asio/detail/chrono.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <sched.h>
#include <source_location>

#include <boost/asio.hpp>

#include "server.hpp"
#include "logger.hpp"

namespace asio = boost::asio;

constexpr std::string_view SERVER_SHUT_DOWN =
	"The server shut down!";
constexpr std::string_view SERVER_STARTED =
	"The server started!";

int main() {
	asio::io_context io{};
	
	try {
		boost::asio::io_context io;
		std::shared_ptr<mydak::server> server = std::make_shared<mydak::server>(io);
		server.get()->start_accepting_connections();
		mydak::log_debug(SERVER_STARTED);
		io.run();
	}
	catch (std::exception& e) {
		mydak::log_debug_error(e.what());
	}

	mydak::log_debug(SERVER_SHUT_DOWN);

	return 0;
}

#endif  // MYDAK_WEBSOCKET_CORE_MAIN_CPP
