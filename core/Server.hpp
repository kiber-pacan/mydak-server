#ifndef MYDAK_WEBSOCKET_CORE_SERVER_HPP
#define MYDAK_WEBSOCKET_CORE_SERVER_HPP

#include <asio.hpp>
#include <map>

struct Connection;

class Server : public std::enable_shared_from_this<Server> {
public:
	Server(asio::io_context& io) : io(io), acceptor(io, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 8888)) {}

	void addClient(std::array<char, 64> publicKey, std::shared_ptr<asio::ip::tcp::socket> socket);

	std::shared_ptr<asio::ip::tcp::socket> getClient(std::array<char, 64> publicKey);

	void startAcceptingConnections();
	
private:
	std::map<std::array<char, 64>, std::shared_ptr<asio::ip::tcp::socket>> clients{};
	
	void handleConnection(std::shared_ptr<Connection> new_connection, const std::error_code& error);

	asio::io_context& io;
	asio::ip::tcp::acceptor acceptor;
};


#endif  // MYDAK_WEBSOCKET_CORE_SERVER_HPP
