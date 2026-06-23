#ifndef MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
#define MYDAK_WEBSOCKET_CORE_CONNECTION_HPP


#include <asio.hpp>
struct Server;

class Connection : public std::enable_shared_from_this<Connection> {
private:
	void handle_write(const std::error_code& error, size_t bytes_transferred);

	std::shared_ptr<asio::ip::tcp::socket> socket;
	std::shared_ptr<Server> server;
	
public:
	Connection(asio::io_context& io, std::shared_ptr<Server> server) :
		socket(std::make_shared<asio::ip::tcp::socket>(io)),
		server(server) {}
		
	std::shared_ptr<asio::ip::tcp::socket> getSocket();

	asio::awaitable<void> recievePublicKey();

	asio::awaitable<void> receiveMessage();

	asio::awaitable<void> keepAlive();
};

#endif  // MYDAK_WEBSOCKET_CORE_CONNECTION_HPP
