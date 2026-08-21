#ifndef MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
#define MYDAK_WEBSOCKET_CORE_CONNECTION_HPP


#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <map>
#include "util/proto.hpp"
#include <boost/asio/awaitable.hpp>

#include "indices.hpp"

namespace mydak {struct server;}

namespace mydak {
	using receive_signal = boost::asio::experimental::channel<void(boost::system::error_code)>;

	struct connection : std::enable_shared_from_this<connection> {
		connection(boost::asio::io_context& io, const std::shared_ptr<server>& server) :
			socket(std::make_shared<boost::asio::ip::tcp::socket>(io)),
			server(server) {}

		std::shared_ptr<boost::asio::ip::tcp::socket> getSocket();

		client_index get_recipient_index(const std::array<char, proto::E2E_KEYS_L>& recipient);

		boost::asio::awaitable<void> start();

		void end_connection() const;

		void delayed_message(std::uint64_t db_index, const std::vector<char>& message) const;
	private:
		client_index indices{};

		std::shared_ptr<boost::asio::ip::tcp::socket> socket;
		std::shared_ptr<server> server;
		std::shared_ptr<receive_signal> signal_channel;
		std::array<char, proto::E2E_KEYS_L> public_key{};
		std::string public_key_string;

		std::map<std::array<char, proto::E2E_KEYS_L>, client_index> clients_cache{};

	};
}
#endif  // MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
