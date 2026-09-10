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
	"DEFAULT_CASE (something may be horribly wrong)";

std::shared_ptr<asio::ip::tcp::socket> mydak::connection::getSocket() {
	return socket;
}

mydak::client_index mydak::connection::get_recipient_index(const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& recipient) {
	return server->get_client_index(recipient);
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

			std::cout << "GREETINGS" << std::endl;

			// Check if prefix is right
			if (greetings[0] != proto::GREETINGS_PREFIX) {
				logger::log_debug_error(FUCKED_UP_GREETNGS_SYMBOL);
				break;
			}

			// Copying message size from greetings
			uint32_t message_size;
			std::memcpy(&message_size, std::span(greetings).subspan(1,4).data(), 4);

			// We get message in little endian
			if constexpr (std::endian::native == std::endian::big) message_size = std::byteswap(message_size);

			std::uint32_t min = 0;
			std::uint32_t max = 512;
			// Checking if message_size is in boundaries
			// TODO FIX
			/*if (message_size > min && message_size < max) {
				logger::log_debug_error(std::format("{} ({})", FUCKED_UP_MESSAGE_SIZE, message_size));
				break;
			}*/

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
			std::cout << "queued_message_size: " << queued_message_size << std::endl;
			#pragma endregion

			queued_message.append_range(size);
			queued_message.append_range(public_key);
			queued_message.append_range(message);
			std::cout << "actual size " << std::size(size) + std::size(public_key) + std::size(message) << std::endl;

			size_t tries = 0;

			// Evil goto
		    add_message_to_queue:

			const client_index recipient_index = get_recipient_index(recipient);


			// If no client with that public key is currently online
			// we add the queued message to the mariadb database
			if (recipient_index.index == client_index::invalid_index) {
				delayed_message(recipient_index.db_index, queued_message);
				continue;
			}


			// Trying to add queued message to the queue and processing the code
			const uint8_t code =
				co_await server->add_message_to_queue(recipient_index.index, recipient_index.generation, queued_message);
			std::cout << static_cast<std::size_t>(code) << std::endl;

			switch (code) {
				// No client with that index
			    case codes::NO_CLIENT: {
					logger::log_debug_error(NO_CLIENT);
					break;
				}
				// Wrong  generation
			    case codes::EXPIRED_CLIENT: {
					logger::log_debug_error(EXPIRED_CACHED_CLIENT);

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
			    	logger::log_func_debug("SUCCESS");
					break;
			    }
				case codes::EXCEPTION: {
					logger::log_func_debug_error("FUCK");
					break;
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
