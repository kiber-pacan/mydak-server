#ifndef MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
#define MYDAK_WEBSOCKET_CORE_CONNECTION_HPP


#include <asio.hpp>
#include <asio/experimental/channel.hpp>
#include <map>
#include "proto.hpp"

namespace mydak {struct server;}

namespace mydak {
	using receive_signal = asio::experimental::channel<void(asio::error_code)>;

	struct connection : public std::enable_shared_from_this<connection> {
		connection(asio::io_context& io, std::shared_ptr<mydak::server> server) :
			socket(std::make_shared<asio::ip::tcp::socket>(io)),
			server(server) {}
		
		std::shared_ptr<asio::ip::tcp::socket> getSocket();
		
		std::optional<size_t> getRecipientIndex(std::array<char, 64> recipient);
	
		asio::awaitable<void> start();
	private:
		size_t clientIndex;
	
		std::shared_ptr<asio::ip::tcp::socket> socket;
		std::shared_ptr<mydak::server> server;
		std::shared_ptr<receive_signal> signal_channel;

		std::map<std::array<char, mydak::proto::PUBLIC_KEY_L>, size_t> clients_cache{};

	};
}
#endif  // MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
