#ifndef MYDAK_WEBSOCKET_CORE_MAIN_CPP
#define MYDAK_WEBSOCKET_CORE_MAIN_CPP


#include <iostream>
#include <asio.hpp>

#include "server.hpp"
#include <source_location>

int main() {
	try {
		asio::io_context io;
		std::shared_ptr<mydak::server> server = std::make_shared<mydak::server>(io);
		server.get()->startAcceptingConnections();
		io.run();
	}
	catch (std::exception& e) {
		std::string func = std::source_location::current().function_name(); 
		// Should have just used __func__, but we rocking with latest c++ standard
			
		std::cout << func  + " : " << e.what() << "\n";
	
	}

	return 0;
}

#endif  // MYDAK_WEBSOCKET_CORE_MAIN_CPP
