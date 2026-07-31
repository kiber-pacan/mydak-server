#include <boost/asio/detached.hpp>
#include <boost/asio/detail/chrono.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <optional>
#include <source_location>
#include "caca++.h"

#include "server.hpp"
#include "connection.hpp"
#include "logger.hpp"

constexpr std::string_view EMPTY_SLOT_OPTIONAL =
	"Somehow slot optional is empty!";
constexpr std::string_view EMPTY_SLOT =
	"Current slot marked as empty!";
constexpr std::string_view NO_ONLINE_CLIENT =
	"No currently online client with that key!";
constexpr std::string_view NO_SLOT_VALUE =
	"Somehow slot has no value, recipient is probably offline!";
constexpr std::string_view WRONG_GENERATION =
	"Generation is wrong! Should get a new client.";

void mydak::server::start_accepting_connections() {
	auto new_connection = std::make_shared<mydak::connection>(io, shared_from_this());
	
	// Wait until server recieves connection 
	acceptor.async_accept(
		*new_connection->getSocket(),
		std::bind(
			&server::handle_connection,
			this,
			new_connection,
			boost::asio::placeholders::error
		)
	);
}

void mydak::server::handle_connection(std::shared_ptr<mydak::connection> new_connection, const std::error_code& error) {
	std::cout << "new connection" << std::endl;

	if (!error) {
		// Spawn coroutine and send it on a free voyage.
		asio::co_spawn(
			io,
			new_connection->start(),
			asio::detached
		);
	}
	
	// Start accepting connections again.
	start_accepting_connections();
}



[[nodiscard]] uint8_t mydak::server::add_message_to_queue(
	size_t sender_index,
	size_t recipient_index,
	size_t generation,
	std::vector<char> message
) {
	auto& slot = clients_slot_vector[recipient_index];
	if (slot.empty()) {
		mydak::log_debug_error(NO_SLOT_VALUE);
		return 0 ;
	}
	if (slot.get_slot_generation() != generation) {
		// TODO GET THIS IN CONNECTION TO UPDATE CACHE
		// OR IF NO CONNECTION WITH THAT PUBLIC ID ADD TO THE
		// NEW CONNECTIONS WATCHLIST
		mydak::log_debug_error(WRONG_GENERATION);
		return 1;
	}
		
	mydak::client& client = slot.get_slot_value();

	const auto& data = client.get_client_data();
	auto& signal_channel = data.signal_channel; 
			
	// Add messages to recipient
	client.add_message(std::move(message));
			
	boost::system::error_code error_code;
	
	// Update recipient socket coroutine
	asio::co_spawn(
		io,
		[signal_channel, error_code]() -> asio::awaitable<void>{			
			co_await signal_channel->async_send(error_code, boost::asio::use_awaitable);
		},
		asio::detached
	);
	
	if (error_code) {
		mydak::log_func_debug_error(error_code.message());
		return 2;
	}
	return 3;
}


size_t mydak::server::add_client(
	std::array<char, mydak::proto::PUBLIC_KEY_L> public_key,
	const std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
	const std::shared_ptr<receive_signal>& signal_channel
) {
	auto it = client_indices.find(public_key);

	// If no client with that public key is registered
	if (it != client_indices.end()) return -1;
		// Creating new client entry in slot vector and getting its index
		size_t index = clients_slot_vector.emplace_back(socket, signal_channel);

		// Adding public key - index association to the map
		client_indices[public_key] = index;
		
	
	//create coroutine for socket!!!! 
	asio::co_spawn(
		io,
		socket_coroutine(signal_channel, index),
		asio::detached
	);
	
	return index;
}


mydak::optional_ref<mydak::slot<mydak::client>> mydak::server::get_client(const size_t& index) {
	if (index < clients_slot_vector.get().size()) return clients_slot_vector[index];
	return std::nullopt;
}		


void mydak::server::remove_client(size_t index, std::array<char, mydak::proto::PUBLIC_KEY_L> public_key) {
	clients_slot_vector.pop(index);
	client_indices.erase(public_key);
}

std::pair<size_t, size_t> mydak::server::get_client_index(std::array<char, mydak::proto::PUBLIC_KEY_L> public_key) {
	auto it = client_indices.find(public_key);
			
	if (it == client_indices.end()) {
		mydak::log_debug_error(NO_ONLINE_CLIENT);
		return std::pair(-1, -1);
	}


	auto& slot = clients_slot_vector[it->second];
	if (slot.empty()) {
		mydak::log_debug_error(NO_SLOT_VALUE);
		// TODO: INVESTIGATE IF THIS SHOULD RETURN FUCKED UP PAIR OR ACTUALLY VALID PAIR (PROBABLY FUCKED UP ONE)
		// why someone need fucking empty client?
		return std::pair(-1, -1);
	}
	
	return std::pair(it->second, slot.get_slot_generation());
}

// TODO MAKE DEGENERATE PROOF
boost::asio::awaitable<void> mydak::server::socket_coroutine(const std::shared_ptr<mydak::receive_signal>& signal_channel, size_t clientIndex) {
	try {
		while (true) {
			co_await signal_channel->async_receive(boost::asio::use_awaitable);
			
			// Get client messages and socket
			mydak::optional_ref<mydak::slot<mydak::client>> slot_optional = get_client(clientIndex);
			if (!slot_optional.has_value()) {
				mydak::log_debug_error(EMPTY_SLOT_OPTIONAL);
				continue;
			}

			auto& slot = slot_optional.value();

			if (slot.empty()) {
				mydak::log_debug_error(EMPTY_SLOT);
				continue;
			}
			
			auto data = slot.get_slot_value().get_client_data();
			
			auto& socket = data.socket;
			auto& messages = data.messages;

			if (socket == nullptr || messages == nullptr) {
				mydak::log_debug_error(std::format("socket is {}, messages = {}", (socket == nullptr) ? "NULL" : "NON NULL", (messages == nullptr) ? "NULL" : "NON NULL"));
				continue;
			}
			
			// Iterate through messages what recipient have
			for (; !messages->empty(); messages->pop())
				co_await boost::asio::async_write(*socket, boost::asio::buffer(messages->front()), boost::asio::use_awaitable);
		}
	} catch (const std::exception& e) {
		mydak::log_debug_error(e.what());
	}

	co_return;
}
