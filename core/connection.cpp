#include <asio/error_code.hpp>
#include <asio/use_awaitable.hpp>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <optional>
#include <string>
#include <asio.hpp>
#include <source_location>
#include <array>


#include "connection.hpp"
#include "server.hpp"
#include "proto.hpp"

   		
std::shared_ptr<asio::ip::tcp::socket> mydak::connection::getSocket() {
	return socket;
}

std::optional<size_t> mydak::connection::getRecipientIndex(std::array<char, mydak::proto::PUBLIC_KEY_L> recipient) {
	size_t index;
	auto it = clients_cache.find(recipient);
	
	if (it == clients_cache.end()) {
		std::optional<size_t> indexOpt = server->getClientIndex(recipient);
		if (indexOpt.has_value()) {
			index = indexOpt.value();
		} else {
			return std::nullopt;
		}
		
		clients_cache[recipient] = index;
	} else {
		index = it->second;
	}

	return index;
}

asio::awaitable<void> mydak::connection::start() {
	// Voodoo type shi to keep alive tcp_connection after it's poiner death
	auto self = shared_from_this();

	asio::socket_base::keep_alive option(true);
	socket->set_option(option);

	signal_channel = std::make_shared<receive_signal>(socket->get_executor());
	
	try {
		std::array<char, mydak::proto::PUBLIC_KEY_L> sender{};
		
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
		std::cout << std::format("{} connected! key: {}", ip.to_string(), std::string(sender.data(), 64)) << std::endl;
			
		// Add that boy to the map
		clientIndex = server->addClient(sender, socket, signal_channel);


		// Recieve messages
		while (true) {
			// GREETINGS
			std::array<char, mydak::proto::GREETINGS_PREFIX_L + mydak::proto::MESSAGE_SIZE_L + mydak::proto::PUBLIC_KEY_L> greetings{};
			co_await asio::async_read(*socket.get(), asio::buffer(greetings, greetings.size()), asio::use_awaitable);

			if (greetings[0] != mydak::proto::GREETINGS_PREFIX) {
				std::cout << "greetings[0] != '\x67'" << std::endl;
				break;
			}

			
			// MESSAGE SIZE
			uint32_t message_size;
			std::memcpy(&message_size, std::span(greetings).subspan(1,4).data(), 4);

			if (message_size < 1) {
				std::cout << "Message size < 1" << std::endl;
				break;
			}

			// RECIPIENT
			std::array<char, mydak::proto::PUBLIC_KEY_L> recipient{};
			std::ranges::copy(std::span(greetings).subspan(5, 64), recipient.begin());
			
			// MESSAGE
			std::vector<char> message{};
			message.resize(message_size);
			co_await asio::async_read(*socket.get(), asio::buffer(message, message_size), asio::use_awaitable);

			
			std::array<char, mydak::proto::MESSAGE_SIZE_L> size =
				std::bit_cast<std::array<char, mydak::proto::MESSAGE_SIZE_L>>(static_cast<uint32_t>(message_size));

			size_t message_with_public_key_size = message_size + mydak::proto::PUBLIC_KEY_L + mydak::proto::MESSAGE_SIZE_L;
			std::vector<char> message_with_public_key{};
			message_with_public_key.reserve(message_with_public_key_size);
			message_with_public_key.append_range(sender);
			message_with_public_key.append_range(size);
			message_with_public_key.append_range(message);

			//std::cout << std::string(message_with_public_key.data(), message_with_public_key.size()) << std::endl;
			
			std::optional<size_t> recipientIndexOpt = getRecipientIndex(recipient);
			if (!recipientIndexOpt.has_value()) {
				std::cout << "No recipient" << std::endl;
				break;
			}
			
			server->addMessageToQueue(clientIndex, recipientIndexOpt.value(), message_with_public_key);
		}
	}
	catch (std::exception& e) {
		std::string func = std::source_location::current().function_name(); 
		// Should have just used __func__, but we rocking with latest c++ standard
		
		std::cout << func  + ":" << e.what() << "\n";
	}
	co_return;
}
