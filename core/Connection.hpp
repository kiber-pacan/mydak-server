#ifndef MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
#define MYDAK_WEBSOCKET_CORE_CONNECTION_HPP


#include <asio.hpp>
#include <asio/experimental/channel.hpp>
#include <map>

struct Server;

using Signal = asio::experimental::channel<void(asio::error_code)>;

class Connection : public std::enable_shared_from_this<Connection> {
private:
	void handle_write(const std::error_code& error, size_t bytes_transferred);

	size_t clientIndex;
	
	std::shared_ptr<asio::ip::tcp::socket> socket;
	std::shared_ptr<Server> server;
	std::shared_ptr<Signal> signal_channel;

	std::map<std::array<char, 64>, size_t> clients_cache{};
public:
	Connection(asio::io_context& io, std::shared_ptr<Server> server) :
		socket(std::make_shared<asio::ip::tcp::socket>(io)),
		server(server) {}
		
	std::shared_ptr<asio::ip::tcp::socket> getSocket();

	std::optional<size_t> getRecipientIndex(std::array<char, 64> recipient);
	
	asio::awaitable<void> start();
};

#endif  // MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
