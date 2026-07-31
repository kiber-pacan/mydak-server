#include <boost/asio/use_awaitable.hpp>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <optional>
#include <string>
#include <boost/asio.hpp>
#include <source_location>
#include <array>
#include <string_view>
#include <cstdint>

#include "connection.hpp"
#include "server.hpp"
#include "proto.hpp"
#include "logger.hpp"

constexpr std::string_view FUCKED_UP_GREETNGS_SYMBOL =
	"First symbol aka greetings[0] is != '\x67'!";
constexpr std::string_view FUCKED_UP_MESSAGE_SIZE =
	"Total message size is less than 1 or more than 512 symbols!";
constexpr std::string_view NO_CLIENT_WITH_THAT_KEY =
	"No client with that key!";
constexpr std::string_view CONNECTION_ENDED =
	"Connection ended!";
constexpr std::string_view NO_CLIENT =
	"No client with that index";
constexpr std::string_view EXPIRED_CACHED_CLIENT =
	"Cached client is expired, getting new one!";
constexpr std::string_view EXPIRED_CACHED_CLIENT_SECOND_TRY =
	"Cached client is still somehow expired after getting new one!";
constexpr std::string_view BAD_SIGNAL =
	"Idk how you managed to fuck with signal channel.";



std::shared_ptr<boost::asio::ip::tcp::socket> mydak::connection::getSocket() {
	return socket;
}

std::optional<std::pair<size_t, size_t>> mydak::connection::get_recipient_index(std::array<char, mydak::proto::PUBLIC_KEY_L> recipient) {
	std::pair<size_t, size_t> pair;
	auto it = clients_cache.find(recipient);
	
	if (it == clients_cache.end()) {
		std::optional<std::pair<size_t, size_t>> index_optional = server->get_client_index(recipient);
		if (index_optional.has_value() and index_optional->first != -1) {
			pair = index_optional.value();
		} else {
			return std::nullopt;
		}
		
		clients_cache[recipient] = pair;
	} else {
		pair = it->second;
	}

	return pair;
}

boost::asio::awaitable<void> mydak::connection::start() {
	// Voodoo type shi to keep coroutine alive after it's poiner death
	auto self = shared_from_this();

	//boost::asio::socket_base::keep_alive option(true);
	//socket->set_option(option);

	signal_channel = std::make_shared<receive_signal>(socket->get_executor());
	
	try {
		std::array<char, mydak::proto::PUBLIC_KEY_L> public_key{};
		
		// Another fucking bullshit.
		//
		//asio::awaitable<size_t> bytes_awaitable = asio::async_read(socket, asio::buffer(rawkey), asio::use_awaitable);
		//size_t bytes = co_await static_cast<asio::awaitable<size_t>&&>(bytes_awaitable); 
		
		
		// Still bullshit, but a little bit prettier.
		boost::asio::awaitable<size_t> bytes_awaitable = boost::asio::async_read(*socket.get(), boost::asio::buffer(public_key), boost::asio::use_awaitable);
		co_await std::move(bytes_awaitable);
			

		// Look at that beauty! Unfortunately asio::awaitable<> dies because it's an rvalue. 
		//
		// size_t bytes = co_await asio::async_read(socket, asio::buffer(key), asio::use_awaitable);
			
			
		boost::asio::ip::address ip = socket->remote_endpoint().address();
			
		// Wow, we got the public key (aka login) from some degenerate. With which we can receive messages from other people.
		mydak::log_debug(std::format("{} connected! key: {}", ip.to_string(), std::string(public_key.data(), 64)));
		
		// Add that boy to the map
		index = server->add_client(public_key, socket, signal_channel);
		this->public_key = public_key;

		// Recieve messages
		while (true) {
			// GREETINGS
			std::array<char, mydak::proto::GREETINGS_PREFIX_L + mydak::proto::MESSAGE_SIZE_L + mydak::proto::PUBLIC_KEY_L> greetings{};
			co_await boost::asio::async_read(*socket.get(), boost::asio::buffer(greetings, greetings.size()), boost::asio::use_awaitable);

			if (greetings[0] != mydak::proto::GREETINGS_PREFIX) {
				mydak::log_debug_error(FUCKED_UP_GREETNGS_SYMBOL);
				break;
			}

			
			// MESSAGE SIZE
			uint32_t message_size;
			std::memcpy(&message_size, std::span(greetings).subspan(1,4).data(), 4);

			// We get message in little endian
			if constexpr (std::endian::native == std::endian::big) message_size = std::byteswap(message_size);

			if (message_size < 1 || message_size > 512) {
				mydak::log_debug_error(std::format("{} ({})", FUCKED_UP_MESSAGE_SIZE, message_size));
				break;
			}

			// RECIPIENT
			std::array<char, mydak::proto::PUBLIC_KEY_L> recipient{};
			std::ranges::copy(std::span(greetings).subspan(5, 64), recipient.begin());
			
			// MESSAGE
			std::vector<char> message{};
			message.resize(message_size);
			co_await boost::asio::async_read(*socket.get(), boost::asio::buffer(message, message_size), boost::asio::use_awaitable);


			// Always get little endian
			std::array<char, mydak::proto::MESSAGE_SIZE_L> size =
				(std::endian::native == std::endian::little) ?
				(std::bit_cast<std::array<char, mydak::proto::MESSAGE_SIZE_L>>(static_cast<uint32_t>(message_size)))
				:
				(std::bit_cast<std::array<char, mydak::proto::MESSAGE_SIZE_L>>(std::byteswap(static_cast<uint32_t>(message_size))));
			
			
			size_t message_with_public_key_size = message_size + mydak::proto::PUBLIC_KEY_L + mydak::proto::MESSAGE_SIZE_L;
			std::vector<char> message_with_public_key{};
			message_with_public_key.reserve(message_with_public_key_size);
			// [public_key][size][message]
			// sender + size is always PUBLIC_KEY_L + 4
			// TODO FIX ENDIANNES
			message_with_public_key.append_range(public_key);
			message_with_public_key.append_range(size);
			message_with_public_key.append_range(message);

			size_t tries = 0;
			// Evil goto
		    add_message_to_queue:
			std::optional<std::pair<size_t, size_t>> recipient_pair_optional = get_recipient_index(recipient);
			if (!recipient_pair_optional.has_value()) {
				delayed_message(recipient, message_with_public_key);
				continue;
			}


			uint8_t code = server->add_message_to_queue(index, recipient_pair_optional.value().first, recipient_pair_optional.value().second, message_with_public_key);
			switch (code) {
				// No client with that index
			    case 0: {
					log_debug_error(NO_CLIENT);
					break;
				}
				// Wrong  generation
			    case 1: {
					log_debug_error(EXPIRED_CACHED_CLIENT);
					clients_cache.erase(recipient);

					// If we somehow got another expired client
					if (tries++ >= 1) {
						log_debug_error(EXPIRED_CACHED_CLIENT_SECOND_TRY);
						break;
					}
					goto add_message_to_queue; // Evil goto hack to try again without expired cached message
				}
				// Signal failure
			    case 2: {
					log_debug_error(BAD_SIGNAL);
					break;
				}
				// Success
			    case 3: {

			    }
			}
		}
	}
	catch (std::exception& e) {
		end_connection();
	}
	
	end_connection();
	
	co_return;
}

void mydak::connection::end_connection() {
	mydak::log_debug_error(CONNECTION_ENDED);

	server->remove_client(index, public_key);
}

// MYSQL SHENANIGANS
void mydak::connection::delayed_message(std::array<char, mydak::proto::PUBLIC_KEY_L> recipient, std::vector<char> message) {
	mydak::log_debug_error(NO_CLIENT_WITH_THAT_KEY);
}
