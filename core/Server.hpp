#ifndef MYDAK_WEBSOCKET_CORE_SERVER_HPP
#define MYDAK_WEBSOCKET_CORE_SERVER_HPP

#include <asio.hpp>
#include <map>
#include <queue>
#include <asio/experimental/channel.hpp>

using Signal = asio::experimental::channel<void(asio::error_code)>;

struct Connection;

class Server : public std::enable_shared_from_this<Server> {
public:
	Server(asio::io_context& io) : io(io), acceptor(io, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 8888)) {}

	
	void startAcceptingConnections();
	
	size_t addClient(
		std::array<char, 64> sender,
		const std::shared_ptr<asio::ip::tcp::socket>& socket,
		const std::shared_ptr<Signal>& signal_channel
	);

	void addMessageToQueue(
		size_t senderIndex,
		size_t recipientIndex,
		std::vector<char> message
	);

	std::optional<size_t> getClientIndex(std::array<char, 64> client);
	
private:
	size_t clientCounter = 0;
	// Basically we can get queued messages for the corresponding socket with public key.
	std::map<std::array<char, 64>, size_t> clients{};

	std::vector<std::shared_ptr<asio::ip::tcp::socket>> sockets{};
	std::vector<std::queue<std::vector<char>>> messages{};
	std::vector<std::shared_ptr<Signal>> signals{};
	
	asio::io_context& io;
	asio::ip::tcp::acceptor acceptor;

	asio::awaitable<void> addMessageToQueueAsync(
		size_t senderIndex,
		size_t recipientIndex,
		std::vector<char> message
	);
	
	void handleConnection(
		std::shared_ptr<Connection> new_connection,
		const std::error_code& error
	);

	
	asio::awaitable<void> socketCoroutine(
		const std::shared_ptr<Signal>& signal_channel,
		size_t clientIndex
	);
};


#endif  // MYDAK_WEBSOCKET_CORE_SERVER_HPP
