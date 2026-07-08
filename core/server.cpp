#include <asio/detail/chrono.hpp>
#include <asio/use_awaitable.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <optional>
#include <source_location>
#include "caca++.h"

#include "server.hpp"
#include "connection.hpp"

void mydak::server::startAcceptingConnections() {
	auto new_connection = std::make_shared<mydak::connection>(io, shared_from_this());
	
	// Wait until server recieves connection 
	acceptor.async_accept(
		*new_connection->getSocket(),
		std::bind(
			&server::handleConnection,
			this,
			new_connection,
			asio::placeholders::error
		)
	);
}

void mydak::server::handleConnection(std::shared_ptr<mydak::connection> new_connection, const std::error_code& error) {
	if (!error) {
		// Spawn coroutine and it send on a free voyage.
		asio::co_spawn(
			io,
			new_connection->start(),
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



size_t mydak::server::addClient(
	std::array<char, mydak::proto::PUBLIC_KEY_L> sender,
	const std::shared_ptr<asio::ip::tcp::socket>& socket,
	const std::shared_ptr<receive_signal>& signal_channel
) {
	// TODO MAKE DEGENERATEPROOF
	clients[sender] = clientCounter;

	sockets.emplace_back(socket);
	messages.emplace_back(std::queue<std::vector<char>>{});
	signals.emplace_back(signal_channel);
	
	//create coroutine for socket!!!! 
	asio::co_spawn(
		io,
		socketCoroutine(signal_channel, clientCounter),
		[](std::exception_ptr e) {
			if (e) {
				std::cout << "Exception while creating socket coroutine!" << std::endl;
			}
		}
	);
	
	clientCounter++;
	return clientCounter - 1;
}

std::optional<size_t> mydak::server::getClientIndex(std::array<char, 64> client) {
	auto it = clients.find(client);
	
	if (it == clients.end()) {
		std::cout << "No client!" << std::endl;
		return std::nullopt;
	}
	
	return it->second;
}


void mydak::server::addMessageToQueue(
	size_t senderIndex,
	size_t recipientIndex,
	std::vector<char> message
) {
	asio::co_spawn(
		io,
		addMessageToQueueAsync(senderIndex, recipientIndex, message),
		[](std::exception_ptr e) {
			if (e) {
				std::cout << "Exception while creating socket coroutine!" << std::endl;
			}
		}
	);
}



asio::awaitable<void> mydak::server::addMessageToQueueAsync(
	size_t senderIndex,
	size_t recipientIndex,
	std::vector<char> message
) {
	auto& signal_channel = signals[recipientIndex]; 

	// Add messages to recipient
	messages[recipientIndex].emplace(std::move(message));
	
	asio::error_code e;
	// Update recipient socket coroutine
	co_await signal_channel->async_send(e, asio::use_awaitable);

	co_return;
}


asio::awaitable<void> mydak::server::socketCoroutine(const std::shared_ptr<mydak::receive_signal>& signal_channel, size_t clientIndex) {
	try {
		while (true) {
			co_await signal_channel->async_receive(asio::use_awaitable);

			// Get client messages and socket
			auto& queue = messages[clientIndex];
			auto& socket = sockets[clientIndex];
			
			// Iterate through messages what recipient have
			for (; !queue.empty(); queue.pop())
				co_await asio::async_write(*socket, asio::buffer(queue.front()), asio::use_awaitable);
		}
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		co_return;
	}
}
