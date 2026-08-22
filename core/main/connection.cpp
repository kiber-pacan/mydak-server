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
#include "codes.hpp"

constexpr std::string_view FUCKED_UP_GREETNGS_SYMBOL =
	"First symbol aka greetings[0] is != \"0x67\"!";
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
constexpr std::string_view DEFAULT_CASE =
	"DEFAULT_CASE";

std::shared_ptr<asio::ip::tcp::socket> mydak::connection::getSocket() {
	return socket;
}

mydak::client_index mydak::connection::get_recipient_index(const std::array<char, proto::E2E_KEYS_L>& recipient) {
	auto it = clients_cache.find(recipient);
	if (it != clients_cache.end() && it->second.index != client_index::invalid_index) {
		return it->second;
	}

	return clients_cache[recipient] = server->get_client_index(recipient);
}


asio::awaitable<void> mydak::connection::start() {
	// Voodoo type shi to keep coroutine alive after it's poiner death
	auto self = shared_from_this();

	//asio::socket_base::keep_alive option(true);
	//socket->set_option(option);

	signal_channel = std::make_shared<receive_signal>(socket->get_executor());

	try {

		// Another fucking bullshit.
		//
		//asio::awaitable<size_t> bytes_awaitable = asio::async_read(socket, asio::buffer(rawkey), asio::use_awaitable);
		//size_t bytes = co_await static_cast<asio::awaitable<size_t>&&>(bytes_awaitable); 
		
		
		// Still bullshit, but a little bit prettier.
		asio::awaitable<size_t> bytes_awaitable = asio::async_read(*socket.get(), asio::buffer(public_key), asio::use_awaitable);
		co_await std::move(bytes_awaitable);
			

		// Look at that beauty! Unfortunately asio::awaitable<> dies because it's an rvalue. 
		//
		// size_t bytes = co_await asio::async_read(socket, asio::buffer(key), asio::use_awaitable);


		asio::ip::address ip = socket->remote_endpoint().address();
			
		// Wow, we got the public key (aka login) from some degenerate. With which we can receive messages from other people.
		logger::log_debug(std::format("{} connected! key: {}", ip.to_string(), std::string(public_key.data(), 64)));

		// Add that boy to the server and the database
		indices = co_await server->add_client(public_key, socket, signal_channel);

		// Send the user delayed messages from the database
		const auto& ex = co_await asio::this_coro::executor;
		asio::co_spawn(ex, server->send_delayed_messages(indices.index, indices.generation, indices.db_index), asio::detached);

		public_key_string = std::string(public_key.data(), public_key.size());

		// Receive messages
		while (true) {
			// GREETINGS
			std::array<char, proto::GREETINGS_PREFIX_L + proto::MESSAGE_SIZE_L + proto::E2E_KEYS_L> greetings{};
			co_await asio::async_read(*socket.get(), asio::buffer(greetings, greetings.size()), asio::use_awaitable);

			if (greetings[0] != proto::GREETINGS_PREFIX) {
				logger::log_debug_error(FUCKED_UP_GREETNGS_SYMBOL);
				break;
			}

			
			// MESSAGE SIZE
			uint32_t message_size;
			std::memcpy(&message_size, std::span(greetings).subspan(1,4).data(), 4);

			// We get message in little endian
			if constexpr (std::endian::native == std::endian::big) message_size = std::byteswap(message_size);

			if (message_size < 1 || message_size > 512) {
				logger::log_debug_error(std::format("{} ({})", FUCKED_UP_MESSAGE_SIZE, message_size));
				break;
			}

			// RECIPIENT
			std::array<char, proto::E2E_KEYS_L> recipient{};
			std::ranges::copy(std::span(greetings).subspan(5, 64), recipient.begin());

			// MESSAGE
			std::vector<char> message{};
			message.resize(message_size);
			co_await asio::async_read(*socket.get(), asio::buffer(message, message_size), asio::use_awaitable);


			// Always get little endian
			std::array<char, proto::MESSAGE_SIZE_L> size =
				(std::endian::native == std::endian::little) ?
				(std::bit_cast<std::array<char, proto::MESSAGE_SIZE_L>>(static_cast<uint32_t>(message_size)))
				:
				(std::bit_cast<std::array<char, proto::MESSAGE_SIZE_L>>(std::byteswap(static_cast<uint32_t>(message_size))));


			const size_t message_with_public_key_size = message_size + proto::E2E_KEYS_L + proto::MESSAGE_SIZE_L;
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

			const client_index recipient_index = get_recipient_index(recipient);

			if (recipient_index.index == client_index::invalid_index) {
				delayed_message(recipient_index.db_index, message_with_public_key);
				continue;
			}


			const uint8_t code = co_await server->add_message_to_queue(recipient_index.index, recipient_index.generation, message_with_public_key);
			switch (code) {
				// No client with that index
			    case codes::NO_CLIENT: {
					logger::log_debug_error(NO_CLIENT);
					break;
				}
				// Wrong  generation
			    case codes::EXPIRED_CLIENT: {
					logger::log_debug_error(EXPIRED_CACHED_CLIENT);
					clients_cache.erase(recipient);

					// If we somehow got another expired client
					if (tries++ >= 1) {
						logger::log_debug_error(EXPIRED_CACHED_CLIENT_SECOND_TRY);
						break;
					}
					goto add_message_to_queue; // Evil goto hack to try again without expired cached message
				}
				// Signal failure
			    case codes::BAD_SIGNAL: {
					logger::log_debug_error(BAD_SIGNAL);
					break;
				}
				// Success
			    case codes::SUCCESS: {

			    }
				default: logger::log_func_debug_error(DEFAULT_CASE);
			}
		}
	}
	catch (const boost::system::system_error& e) {
		logger::exception_func(e);
		end_connection();
		co_return;
	}

	end_connection();

	co_return;
}

void mydak::connection::end_connection() const {
	logger::log_debug_error(CONNECTION_ENDED);

	server->remove_client(indices.index, public_key);
}

// MYSQL SHENANIGANS
void mydak::connection::delayed_message(const std::uint64_t db_index, const std::vector<char>& message) const {
	// Should be initialized because we called get_recipient_index and cashed its output
	server->add_message_to_db(db_index, message);
}
