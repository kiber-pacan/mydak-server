#ifndef MYDAK_WEBSOCKET_CORE_SERVER_HPP
#define MYDAK_WEBSOCKET_CORE_SERVER_HPP


#include <boost/asio.hpp>
#include <cstddef>
#include <map>
#include <queue>
#include <boost/asio/experimental/channel.hpp>
#include <optional>
#include <boost/asio/awaitable.hpp>

#include "util/proto.hpp"
#include "optional_ref.hpp"
#include "slot.hpp"
#include "client.hpp"
#include "slot_vector.hpp"

namespace mydak {struct connection;}
namespace mydak {
	using receive_signal = asio::experimental::channel<void(boost::system::error_code)>;

	class server : public std::enable_shared_from_this<server> {
	public:
		explicit server(asio::io_context& io) : io(io), acceptor(io, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 8888)) {}

	
		void start_accepting_connections();

		// Returns index of client
		[[nodiscard]] size_t add_client(
			const std::array<char, proto::PUBLIC_KEY_L> &client,
			const std::shared_ptr<asio::ip::tcp::socket>& socket,
			const std::shared_ptr<receive_signal>& signal_channel
		);

		optional_ref<slot<client>> get_client(
			const size_t& index
		);

		void remove_client(
			size_t index,
			const std::array<char, proto::PUBLIC_KEY_L> &public_key
		);

		// Returns 0 if no client, 1 if wrong generation, 2 if failed to send signal, 3 if message sent
		[[nodiscard]] uint8_t add_message_to_queue(
			size_t recipient_index,
			size_t generation,
			const std::vector<char>& message
		);
		
		std::pair<size_t, size_t> get_client_index(const std::array<char, proto::PUBLIC_KEY_L>& public_key);
	private:
		//mydak::clients clients{};		
		slot_vector<client> clients_slot_vector{};
		std::map<std::array<char, proto::PUBLIC_KEY_L>, size_t> client_indices{};

		
		asio::io_context& io;
		asio::ip::tcp::acceptor acceptor;
		
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
