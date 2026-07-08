#ifndef MYDAK_WEBSOCKET_CORE_PROTOCOL_HPP
#define MYDAK_WEBSOCKET_CORE_PROTOCOL_HPP

#include <cstddef>

namespace mydak {
	// Mydak protocol
	struct proto {
		static constexpr size_t PUBLIC_KEY_L = 64;

		static constexpr size_t MESSAGE_SIZE_L = 4;
		
		static constexpr char GREETINGS_PREFIX = 0x67;
		static constexpr char GREETINGS_PREFIX_L = 1;
	};
}
#endif  // MYDAK_WEBSOCKET_CORE_PROTOCOL_HPP
