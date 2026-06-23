#include <functional>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include "Server.hpp"
#include "Connection.hpp"



void Server::startAcceptingConnections() {
	auto new_connection = std::make_shared<Connection>(Connection(io, shared_from_this()));
	
	// Wait until server recieves connection 
	acceptor.async_accept(
		*new_connection->getSocket().get(),
		std::bind(
			&Server::handleConnection,
			this,
			new_connection,
			asio::placeholders::error
		)
	);
}

void Server::addClient(std::array<char, 64> publicKey, std::shared_ptr<asio::ip::tcp::socket> socket) {
	// TODO MAKE DEGENERATEPROOF
	clients[publicKey] = socket;
	//for (auto &[publicKey, socket] : clients) {
	//	std::cout << std::format("{} : {}", publicKey, socket.to_string()) << std::endl;
	//}
}

std::shared_ptr<asio::ip::tcp::socket> Server::getClient(std::array<char, 64> publicKey) {
	return clients[publicKey];
}

void Server::handleConnection(std::shared_ptr<Connection> new_connection, const std::error_code& error) {
	if (!error) {
		// Spawn coroutine and it send on a free voyage.
		asio::co_spawn(
			io,
			new_connection->recievePublicKey(),
			[](std::exception_ptr e) {
				if (e) {
					std::cout << "Exception while handling accept!" << std::endl;
				}
			}
		);
	}
	
	// Start accepting connections again.
	startAcceptingConnections();
}
