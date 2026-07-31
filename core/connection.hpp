#ifndef MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
#define MYDAK_WEBSOCKET_CORE_CONNECTION_HPP


#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <map>
#include "proto.hpp"
#include <boost/asio/awaitable.hpp>

namespace mydak {struct server;}

namespace mydak {
	using receive_signal = boost::asio::experimental::channel<void(boost::system::error_code)>;

	struct connection : public std::enable_shared_from_this<connection> {
		connection(boost::asio::io_context& io, std::shared_ptr<mydak::server> server) :
			socket(std::make_shared<boost::asio::ip::tcp::socket>(io)),
			server(server) {}
		
		std::shared_ptr<boost::asio::ip::tcp::socket> getSocket();
		
		std::optional<std::pair<size_t, size_t>> get_recipient_index(std::array<char, 64> recipient);
	
		boost::asio::awaitable<void> start();

		void end_connection();

		void delayed_message(std::array<char, mydak::proto::PUBLIC_KEY_L> recipient, std::vector<char> message_with_public_key);
	private:
		size_t index;
	
		std::shared_ptr<boost::asio::ip::tcp::socket> socket;
		std::shared_ptr<mydak::server> server;
		std::shared_ptr<receive_signal> signal_channel;
		std::array<char, mydak::proto::PUBLIC_KEY_L> public_key;

		std::map<std::array<char, mydak::proto::PUBLIC_KEY_L>, std::pair<size_t, size_t>> clients_cache{};

	};
}
#endif  // MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
