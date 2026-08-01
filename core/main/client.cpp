#include <format>
#include <cstdint>
#include <vector>
#include <queue>
#include <memory>
#include <iostream>
#include <string>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>


#include "client.hpp"
#include "logger.hpp"

namespace asio = boost::asio;


constexpr std::string_view DATA_ADD_NON_NULL_DATA_INACTIVE =
	"Tried to add data to inactive client with non null data! (previous data is replaced with new)";
constexpr std::string_view DATA_ADD_ACTIVE_CLIENT =
	"Tried to add data to active client spot!";
constexpr std::string_view MESSAGE_ADD_INACTIVE_CLIENT =
	"Tried to add message to inactive client!";
constexpr std::string_view MESSAGE_ADD_NULL_QUEUE =
	"Tried to add message to client with nullptr queue!";

void mydak::client::add_client_data(
	const std::shared_ptr<asio::ip::tcp::socket>& socket,
	const std::shared_ptr<receive_signal>& signal_channel
) {
	bool null_data = data.socket == nullptr && data.signal_channel == nullptr && data.messages == nullptr;
	if (!active) {
		if (!null_data) {
			logger::log_debug_error(DATA_ADD_NON_NULL_DATA_INACTIVE);
		}
		
		data.socket = socket;
		if (data.messages == nullptr) data.messages = std::make_shared<std::queue<std::vector<char>>>();
		data.signal_channel = signal_channel;

		active = true;
		
	} else {
		logger::log_debug_error(DATA_ADD_ACTIVE_CLIENT);
	}
}

void mydak::client::add_message(const std::vector<char>& message) const {
	if (data.messages != nullptr && active)
		data.messages->emplace(message);
			
	if (!active) {
		logger::log_debug_error(MESSAGE_ADD_INACTIVE_CLIENT);
	} else if (data.messages == nullptr) {
		logger::log_debug_error(MESSAGE_ADD_NULL_QUEUE);
	}
}
		
void mydak::client::remove_client() {
	active = false;

	data.messages = nullptr;
	data.signal_channel = nullptr;
	data.socket = nullptr;
}

const mydak::client_data& mydak::client::get_client_data() {
	return data;
}
