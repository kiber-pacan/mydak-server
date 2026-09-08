#ifndef MYDAK_WEBSOCKET_CORE_SERVER_HPP
#define MYDAK_WEBSOCKET_CORE_SERVER_HPP


#include <boost/asio.hpp>
#include <cstddef>
#include <map>
#include <queue>
#include <boost/asio/experimental/channel.hpp>
#include <optional>
#include <boost/asio/awaitable.hpp>

#include "proto.hpp"
#include "optional_ref.hpp"
#include "slot.hpp"
#include "client.hpp"
#include "database.hpp"
#include "indices.hpp"
#include "slot_vector.hpp"

#include "parameters_accessor.hpp"


namespace mydak {struct connection;}
namespace mydak {
	using receive_signal = asio::experimental::channel<void(boost::system::error_code)>;

	class server : public std::enable_shared_from_this<server> {
	public:
		explicit server(asio::io_context& io, const int argc, char* argv[])
		:
		io(io),
		acceptor(io, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 8888)),
		parameters(argc, argv),
		db(io, parameters.get<"--db-hostname">(), parameters.get<"--db-username">(), parameters.get<"--db-password">())
		{}


		void start_accepting_connections();

		// Returns index of client
		[[nodiscard]] asio::awaitable<client_index> add_client(
			const std::array<unsigned char, proto::E2E_KEYS_RAW_L> &public_key,
			const std::shared_ptr<asio::ip::tcp::socket>& socket,
			const std::shared_ptr<receive_signal>& signal_channel
		);

		optional_ref<slot<client>> get_client(
			const size_t& index
		);

		void remove_client(
			size_t index,
			const std::array<unsigned char, proto::E2E_KEYS_RAW_L> &public_key
		);

		// Returns 0 if no client, 1 if wrong generation, 2 if failed to send signal, 3 if message sent
		[[nodiscard]] asio::awaitable<uint8_t> add_message_to_queue(
			size_t recipient_index,
			size_t generation,
			const std::vector<char>& message
		);

		client_index get_client_index(const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& public_key);


		asio::awaitable<std::uint64_t> add_client_to_db(
			const std::array<unsigned char, proto::E2E_KEYS_RAW_L>& public_key
		);

		void add_message_to_db(
			std::uint64_t index,
			const std::vector<char>& message
		);

		asio::awaitable<void> send_delayed_messages(
			std::size_t recipient_index,
			std::size_t generation,
			std::uint64_t db_index
		);
	private:
		asio::awaitable<void> add_message_to_db_internal(
			std::uint64_t index,
			const std::vector<char>& message
		);

		slot_vector<client> clients_slot_vector{};

		struct array_hasher {
			template <std::size_t N>
			std::size_t operator()(const std::array<unsigned char, N> array) const noexcept {
				std::size_t seed = 0;
				for (const auto c : array) {
					seed ^= std::hash<unsigned char>{}(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
				return seed;
			}
		};
		std::unordered_map<const std::array<unsigned char, proto::E2E_KEYS_RAW_L>, client_index, array_hasher> client_indices{};

		asio::io_context& io;
		asio::ip::tcp::acceptor acceptor;
		args::parameters_accessor parameters;
		database db;

		void handle_connection(
			const std::shared_ptr<connection>& new_connection,
			const std::error_code& error
		);


		asio::awaitable<void> socket_coroutine(
			const std::shared_ptr<receive_signal>& signal_channel,
			size_t clientIndex
		);
	};

}
#endif  // MYDAK_WEBSOCKET_CORE_SERVER_HPP