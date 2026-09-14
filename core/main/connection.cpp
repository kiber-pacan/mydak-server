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

constexpr std::string_view FUCKED_UP_GREETINGS_SYMBOL =
	"First symbol aka greetings[0] is != \"0x67\"!";
constexpr std::string_view FUCKED_UP_MESSAGE_SIZE =
	"Total message size is less not in bounds!";
constexpr std::string_view NO_CLIENT_WITH_THAT_KEY =
	"No client with that key!";
constexpr std::string_view CONNECTION_ENDED =
	"Connection ended!";



asio::awaitable<void> mydak::connection::start() {
	// Voodoo type shi to keep coroutine alive after it's poiner death
	auto self = shared_from_this();

	signal_channel = std::make_shared<receive_signal>(socket->get_executor());

	try {
		// Still bullshit, but a little bit prettier.
		asio::awaitable<size_t> bytes_awaitable = asio::async_read(*socket.get(), asio::buffer(public_key), asio::use_awaitable);
		co_await std::move(bytes_awaitable);

		asio::ip::address ip = socket->remote_endpoint().address();
			
		// Wow, we got the public key (aka login) from some degenerate. With which we can receive messages from other people.
		logger::log_debug(std::format("{} connected! key: {}", ip.to_string(), tools::bin2hex_string(public_key)));

		// Add that boy to the server and the database
		indices = co_await server->add_client(public_key, socket, signal_channel);

		// Send the user delayed messages from the database
		const auto& ex = co_await asio::this_coro::executor;
		asio::co_spawn(ex, server->send_delayed_messages(indices.index, indices.generation, indices.db_index), asio::detached);

		public_key_string = std::string(reinterpret_cast<const char *>(public_key.data()), public_key.size());

		// Receive messages
		while (true) {
			// GREETINGS [0x67][message size][recipient]
			std::array<char, proto::GREETINGS_PREFIX_L + proto::MESSAGE_SIZE_L + proto::E2E_KEYS_RAW_L> greetings{};
			co_await asio::async_read(*socket.get(), asio::buffer(greetings, greetings.size()), asio::use_awaitable);

			// Check if prefix is right
			if (greetings[0] != proto::GREETINGS_PREFIX) {
				logger::log_debug_error(FUCKED_UP_GREETINGS_SYMBOL);
				break;
			}

			// Copying message size from greetings
			uint32_t message_size;
			std::memcpy(&message_size, greetings.data() + 1, 4);

			// We get message in little endian
			if constexpr (std::endian::native == std::endian::big) message_size = std::byteswap(message_size);

			// Checking if message_size is in boundaries
			if (message_size < proto::MIN_MESSAGE_SIZE || message_size > proto::MAX_MESSAGE_SIZE) {
				logger::log_debug_error(std::format("{} ({})", FUCKED_UP_MESSAGE_SIZE, message_size));
				break;
			}

			// Copying recipient into array from greetings
			constexpr std::size_t message_start = proto::GREETINGS_PREFIX_L + proto::MESSAGE_SIZE_L;
			std::array<unsigned char, proto::E2E_KEYS_RAW_L> recipient{};
			memcpy(
				recipient.data(),
				greetings.data() + message_start,
				std::size(greetings) - message_start
			);

			// Receiving message
			std::vector<char> message{};
			message.resize(message_size);
			co_await asio::async_read(*socket.get(), asio::buffer(message, message_size), asio::use_awaitable);


			// [message size][public key][message]
			const size_t queued_message_size = proto::MESSAGE_SIZE_L + proto::E2E_KEYS_RAW_L + message_size;
			std::vector<char> queued_message{};
			queued_message.reserve(queued_message_size);

			#pragma region Message size
			// We receive message in little-endian,
			// so we should byte swap it to big-endian when system is not little-endian
			std::array<char, proto::MESSAGE_SIZE_L> size;  // NOLINT(*-pro-type-member-init)
			if constexpr (std::endian::native != std::endian::little) {
				size = std::bit_cast<std::array<char, proto::MESSAGE_SIZE_L>>(std::byteswap(static_cast<uint32_t>(message_size)));
			} else {
				size = std::bit_cast<std::array<char, proto::MESSAGE_SIZE_L>>(static_cast<uint32_t>(message_size));
			}
			#pragma endregion

			queued_message.append_range(size);
			queued_message.append_range(public_key);
			queued_message.append_range(message);

			size_t tries = 0;

			#pragma region Sending
			// Evil goto
		    add_message_to_queue:

			const client_index recipient_index = server->get_client_index(recipient);

			// If no client with that public key is currently online
			// we add the queued message to the mariadb database
			if (recipient_index.index == client_index::invalid_index) {
				delayed_message(recipient_index.db_index, queued_message);
				continue;
			}

			// Trying to add queued message to the queue and processing the code
			const auto code =
				co_await server->add_message_to_queue(recipient_index.index, recipient_index.generation, queued_message);

			switch (code) {
				// No client with that index
			    case send_codes::NO_CLIENT: {
					logger::log_debug_error(send_strings::NO_CLIENT);
					break;
				}
				// Wrong  generation
			    case send_codes::EXPIRED_CLIENT: {
					logger::log_debug_error(send_strings::EXPIRED_CACHED_CLIENT);

					// If we somehow got another expired client
					if (tries++ >= 2) {
						logger::log_debug_error(send_strings::EXPIRED_CACHED_CLIENT_SECOND_TRY);
						break;
					}
					goto add_message_to_queue; // Evil goto hack to try again without expired cached message
				}
				// Signal failure
			    case send_codes::BAD_SIGNAL: {
					logger::log_debug_error(send_strings::BAD_SIGNAL);
					break;
				}
				// Success
			    case send_codes::SUCCESS: {}
				case send_codes::EXCEPTION: {}
			}
			#pragma endregion
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
