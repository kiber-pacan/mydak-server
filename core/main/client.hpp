 #ifndef MYDAK_WEBSOCKET_CLIENT_CLIENT_HPP
#define MYDAK_WEBSOCKET_CLIENT_CLIENT_HPP


#include <queue>
#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>
#include "string_view"
#include "memory"
#include "vector"

namespace asio = boost::asio;

namespace mydak {
	using receive_signal = asio::experimental::channel<void(boost::system::error_code)>;

	struct client_data {
		client_data(
			std::shared_ptr<asio::ip::tcp::socket> socket,
			std::shared_ptr<receive_signal> signal_channel
		) : socket(std::move(socket)), messages(std::make_shared<std::queue<std::vector<char>>>()), signal_channel(std::move(signal_channel)) {}
			
		std::shared_ptr<asio::ip::tcp::socket> socket{};
		std::shared_ptr<std::queue<std::vector<char>>> messages{};
		std::shared_ptr<receive_signal> signal_channel{};
	};

	struct client {		
		client(
			const std::shared_ptr<asio::ip::tcp::socket>& socket,
			const std::shared_ptr<receive_signal>& signal_channel
		) : active(true), data(socket, signal_channel) {}
 		
		bool active = false;
		
		void add_client_data(
			const std::shared_ptr<asio::ip::tcp::socket>& socket,
			const std::shared_ptr<receive_signal>& signal_channel
		);
		
		void add_message(
			const std::vector<char>& message
		) const;
		
		void remove_client();

		const mydak::client_data& get_client_data();
		
	private:
		mydak::client_data data;
	};

}

#endif  // MYDAK_WEBSOCKET_CLIENT_CLIENT_HPP
