#include <boost/asio.hpp>

#include "coh.hpp"
#include "main/server.hpp"
#include "logger.hpp"

namespace asio = boost::asio;

constexpr std::string_view SERVER_SHUT_DOWN =
	"The server shut down!";
constexpr std::string_view SERVER_STARTED =
	"The server started!";


int main(const int argc, char* argv[]) {
	asio::io_context& io = mydak::coh::io();

	try {
		const auto server = std::make_shared<mydak::server>(io, argc, argv);
		server.get()->start_accepting_connections();
		mydak::logger::log_debug(SERVER_STARTED);
		io.run();
	}
	catch (const boost::system::system_error& e) {
		mydak::logger::exception_func(e);
	}

	mydak::logger::log_debug(SERVER_SHUT_DOWN);

	return 0;
}