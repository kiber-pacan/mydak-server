#include <asio/use_awaitable.hpp>
#include <cstddef>
#include <ctime>
#include <iostream>
#include <string>
#include <asio.hpp>
#include <source_location>
#include "Connection.hpp"
#include "Server.hpp"


void Connection::handle_write(const std::error_code& error, size_t bytes_transferred) {
}
   		
std::shared_ptr<asio::ip::tcp::socket> Connection::getSocket() {
	return socket;
}

asio::awaitable<void> Connection::recievePublicKey() {
	// Voodoo type shi to keep alive tcp_connection after it's poiner death
	auto self = shared_from_this();

	asio::socket_base::keep_alive option(true);
	socket->set_option(option);

	
	try {
		std::array<char, 64> key{};
		
		// Another fucking bullshit.
		/*
		  asio::awaitable<size_t> bytes_awaitable = asio::async_read(socket, asio::buffer(rawkey), asio::use_awaitable);
		  size_t bytes = co_await static_cast<asio::awaitable<size_t>&&>(bytes_awaitable); 
		*/
		
		// Still bullshit, but a little bit prettier.
		asio::awaitable<size_t> bytes_awaitable = asio::async_read(*socket.get(), asio::buffer(key), asio::use_awaitable);
		co_await std::move(bytes_awaitable);
			

		// Look at that beauty! Unfortunately asio::awaitable<> dies because it's an rvalue. 
		// size_t bytes = co_await asio::async_read(socket, asio::buffer(key), asio::use_awaitable);
			
			
		asio::ip::address ip = socket->remote_endpoint().address();
			
		// Wow, we got the public key (aka login) from some degenerate. With which we can receive messages from other people.
		std::cout << std::format("Received {} key from {}!", std::string(key.data(), 64), ip.to_string()) << std::endl;
			
		// Add that boy to the map
		server.get()->addClient(key, socket);

		//co_await asio::async_write(socket, asio::buffer("FUCK"), asio::use_awaitable);
		while (true) {
			std::vector<char> message{};
			co_await asio::async_read_until(*socket.get(), asio::dynamic_buffer(message), "\r\n", asio::use_awaitable);

			std::array<char, 64> recipient{};
			std::copy_n(message.begin(), 64, recipient.begin());
			
			co_await asio::async_write(*server->getClient(recipient).get(), asio::buffer(message), asio::use_awaitable);
		}
	}
	catch (std::exception& e) {
		std::string func = std::source_location::current().function_name(); 
		// Should have just used __func__, but we rocking with latest c++ standard
		
		std::cout << func  + ":" << e.what() << "\n";
	}
	co_return;
}

asio::awaitable<void> Connection::keepAlive() {
	try {
	}
	catch (std::exception& e) {
		std::string func = std::source_location::current().function_name(); 
			
		std::cout << func  + ":" << e.what() << "\n";
	}
	co_return;
}
