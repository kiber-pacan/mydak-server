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
#include "codes.hpp"

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
constexpr std::string_view NO_CLIENT_IN_DB =
	"No client with such public key is in the database.";


void mydak::server::start_accepting_connections() {
	auto new_connection = std::make_shared<connection>(io, shared_from_this());

	// Wait until server receives connection
	acceptor.async_accept(
		*new_connection->getSocket(),
		std::bind(
			&server::handle_connection,
			this,
			new_connection,
			asio::placeholders::error
		)
	);
}

void mydak::server::handle_connection(const std::shared_ptr<connection>& new_connection, const std::error_code& error) {
	logger::log_debug("new connection");

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



asio::awaitable<uint8_t> mydak::server::add_message_to_queue(
	const std::size_t recipient_index,
	const std::size_t generation,
	const std::vector<char>& message
) {
	auto self = shared_from_this(); // Preventing from destroying after getting out of scope

	auto& slot = clients_slot_vector[recipient_index];
	if (slot.empty()) {
		logger::log_debug_error(NO_SLOT_VALUE);
		co_return codes::NO_CLIENT;
	}
	if (slot.get_slot_generation() != generation) {
		// TODO GET THIS IN CONNECTION TO UPDATE CACHE
		// OR IF NO CONNECTION WITH THAT PUBLIC ID ADD TO THE
		// NEW CONNECTIONS WATCHLIST
		logger::log_debug_error(WRONG_GENERATION);
		co_return codes::EXPIRED_CLIENT;
	}
		
	client& client = slot.get_slot_value();

	const auto& data = client.get_client_data();
	auto& signal_channel = data.signal_channel; 
			
	// Add messages to recipient
	client.add_message(message);
			
	boost::system::error_code error_code;
	
	// Update recipient socket coroutine
	co_await signal_channel->async_send(error_code, asio::use_awaitable);
	
	if (error_code) {
		logger::log_func_debug_error(error_code.message());
		co_return codes::BAD_SIGNAL;
	}
	co_return codes::SUCCESS;
}



asio::awaitable<mydak::client_index> mydak::server::add_client(
	const std::array<char, proto::PUBLIC_KEY_L>& public_key,
	const std::shared_ptr<asio::ip::tcp::socket>& socket,
	const std::shared_ptr<receive_signal>& signal_channel
) {
	auto it = client_indices.find(public_key);

	// If client with that public key is already registered
	if (it != client_indices.end()) co_return client_index::empty();

	// Add that boy to the db
	const std::uint64_t db_index = co_await add_client_to_db(public_key);

	// Creating new client entry in slot vector and getting its index
	client_index indices = clients_slot_vector.emplace_back(socket, signal_channel);
	indices.db_index = db_index;

	// Adding public key - index association to the map
	client_indices[public_key] = client_index(indices);


	//create coroutine for socket!!!!
	asio::co_spawn(
		io,
		socket_coroutine(signal_channel, indices.index),
		asio::detached
	);

	co_return indices;
}


mydak::optional_ref<mydak::slot<mydak::client>> mydak::server::get_client(const size_t& index) {
	if (index < clients_slot_vector.get().size()) return clients_slot_vector[index];
	return std::nullopt;
}		


void mydak::server::remove_client(const size_t index, const std::array<char, proto::PUBLIC_KEY_L> &public_key) {
	clients_slot_vector.pop(index);
	client_indices.erase(public_key);
}

mydak::client_index mydak::server::get_client_index(const std::array<char, proto::PUBLIC_KEY_L> &public_key) {
	auto it = client_indices.find(public_key);


	if (it == client_indices.end()) {
		logger::log_debug(NO_ONLINE_CLIENT);
		return {client_index::invalid_index, client_index::invalid_index, db.get_db_index(public_key)};
	}


	auto& slot = clients_slot_vector[it->second.index];
	if (slot.empty()) {
		logger::log_debug_error(NO_SLOT_VALUE);
		return client_index::empty();
	}
	
	return {it->second.index, slot.get_slot_generation(), it->second.db_index};
}

#pragma region Database
asio::awaitable<std::uint64_t> mydak::server::add_client_to_db(
	const std::array<char, proto::PUBLIC_KEY_L>& public_key
) {
	auto it = client_indices.find(public_key);
	const std::uint64_t db_index = co_await db.add_user(public_key);

	if (it != client_indices.end()) {
		it->second.db_index = db_index;
	} else {
		client_indices[public_key] = {client_index::invalid_index, db_index};
	}

	co_return db_index;
}



void mydak::server::add_message_to_db(
	std::uint64_t index,
	const std::vector<char>& message
) {
	if (index == client_index::invalid_index) logger::log_func_debug_error(NO_CLIENT_IN_DB);

	asio::co_spawn(
		io,
		add_message_to_db_internal(index, message),
		asio::detached
	);
}

asio::awaitable<void> mydak::server::add_message_to_db_internal(
	const std::uint64_t index,
	const std::vector<char>& message
) {
	co_await db.add_message(index, message);
}


asio::awaitable<void> mydak::server::send_delayed_messages(
	const std::size_t recipient_index,
	const std::size_t generation,
	const std::uint64_t db_index
) {
	auto self = shared_from_this();
	const auto messages = co_await db.get_delayed_messages(db_index);

	std::vector<std::uint64_t> db_indices{};
	db_indices.reserve(messages.size());

	for (const auto& message : messages) {
		const std::uint8_t code = co_await add_message_to_queue(recipient_index, generation, message.data);

		if (code == codes::SUCCESS) db_indices.push_back(message.db_index);
		else break;
	}

	co_await db.delete_delayed_messages(db_indices);
}
#pragma endregion


asio::awaitable<void> mydak::server::socket_coroutine(const std::shared_ptr<receive_signal>& signal_channel, const size_t clientIndex) {
	try {
		while (true) {
			co_await signal_channel->async_receive(asio::use_awaitable);
			
			// Get client messages and socket
			optional_ref<slot<client>> slot_optional = get_client(clientIndex);
			if (!slot_optional.has_value()) {
				logger::log_debug_error(EMPTY_SLOT_OPTIONAL);
				continue;
			}

			auto& slot = slot_optional.value();

			if (slot.empty()) {
				logger::log_debug_error(EMPTY_SLOT);
				continue;
			}
			
			auto data = slot.get_slot_value().get_client_data();
			
			auto& socket = data.socket;
			auto& messages = data.messages;

			if (socket == nullptr || messages == nullptr) {
				logger::log_debug_error(std::format("socket is {}, messages = {}", socket == nullptr ? "NULL" : "NON NULL", messages == nullptr ? "NULL" : "NON NULL"));
				continue;
			}
			
			// Iterate through messages what recipient have
			for (; !messages->empty(); messages->pop())
				co_await boost::asio::async_write(*socket, asio::buffer(messages->front()), asio::use_awaitable);
		}
	} catch (const boost::system::system_error& e) {
		logger::exception_func(e);
	}

	co_return;
}
