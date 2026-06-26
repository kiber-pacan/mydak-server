#include <asio/error_code.hpp>
#include <asio/use_awaitable.hpp>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <string>
#include <asio.hpp>
#include <source_location>
#include "Connection.hpp"
#include "Server.hpp"
#include <array>


void Connection::handle_write(const std::error_code& error, size_t bytes_transferred) {
}
   		
std::shared_ptr<asio::ip::tcp::socket> Connection::getSocket() {
	return socket;
}

std::optional<size_t> Connection::getRecipientIndex(std::array<char, 64> recipient) {
	size_t index;
	auto it = clients_cache.find(recipient);
	
	if (it == clients_cache.end()) {
		std::optional<size_t> indexOpt = server->getClientIndex(recipient);
		if (indexOpt.has_value()) {
			index = indexOpt.value();
		}
		
		clients_cache[recipient] = index;
	} else {
		index = it->second;
	}

	return index;
}

asio::awaitable<void> Connection::start() {
	// Voodoo type shi to keep alive tcp_connection after it's poiner death
	auto self = shared_from_this();

	asio::socket_base::keep_alive option(true);
	socket->set_option(option);

	signal_channel = std::make_shared<Signal>(socket->get_executor());
	
	try {
		std::array<char, 64> sender{};
		
		// Another fucking bullshit.
		//
		//asio::awaitable<size_t> bytes_awaitable = asio::async_read(socket, asio::buffer(rawkey), asio::use_awaitable);
		//size_t bytes = co_await static_cast<asio::awaitable<size_t>&&>(bytes_awaitable); 
		
		
		// Still bullshit, but a little bit prettier.
		asio::awaitable<size_t> bytes_awaitable = asio::async_read(*socket.get(), asio::buffer(sender), asio::use_awaitable);
		co_await std::move(bytes_awaitable);
			

		// Look at that beauty! Unfortunately asio::awaitable<> dies because it's an rvalue. 
		//
		// size_t bytes = co_await asio::async_read(socket, asio::buffer(key), asio::use_awaitable);
			
			
		asio::ip::address ip = socket->remote_endpoint().address();
			
		// Wow, we got the public key (aka login) from some degenerate. With which we can receive messages from other people.
		std::cout << std::format("Received {} key from {}!", std::string(sender.data(), 64), ip.to_string()) << std::endl;
			
		// Add that boy to the map
		clientIndex = server.get()->addClient(sender, socket, signal_channel);


		// Recieve messages
		while (true) {
			// GREETINGS
			std::array<char, 1 + 4 + 64> greetings{};
			co_await asio::async_read(*socket.get(), asio::buffer(greetings, greetings.size()), asio::use_awaitable);

			if (greetings[0] != '\x67') continue; // Greetings type is 67.

			// MESSAGE SIZE
			uint32_t message_size;
			std::memcpy(&message_size, std::span(greetings).subspan(1,4).data(), 4);

			if (message_size < 1) continue;


			// RECIPIENT
			std::array<char, 64> recipient{};
			std::ranges::copy(std::span(greetings).subspan(5, 64), recipient.begin());
			
			// MESSAGE
			std::vector<char> message{};
			co_await asio::async_read(*socket.get(), asio::buffer(message, message_size), asio::use_awaitable);
			
			
			std::optional<size_t> recipientIndexOpt = getRecipientIndex(recipient);
			if (!recipientIndexOpt.has_value()) continue;
			
			
			server->addMessageToQueue(clientIndex, recipientIndexOpt.value(), message);
		}
	}
	catch (std::exception& e) {
		std::string func = std::source_location::current().function_name(); 
		// Should have just used __func__, but we rocking with latest c++ standard
		
		std::cout << func  + ":" << e.what() << "\n";
	}
	co_return;
}
