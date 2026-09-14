#ifndef MYDAK_BACKEND_CORE_PROTO_HPP
#define MYDAK_BACKEND_CORE_PROTO_HPP

namespace mydak {
	// Mydak protocol
	struct proto {
		static constexpr size_t E2E_KEYS_RAW_L = 32;
		static constexpr size_t E2E_KEYS_HEX_L = E2E_KEYS_RAW_L * 2;

		static constexpr size_t MESSAGE_SIZE_L = 4;
		
		static constexpr char GREETINGS_PREFIX = 0x67;
		static constexpr char GREETINGS_PREFIX_L = 1;

		static constexpr std::size_t MIN_MESSAGE_SIZE = 1;
		static constexpr std::size_t MAX_MESSAGE_SIZE = 1024;
	};
}
#endif  // MYDAK_BACKEND_CORE_PROTO_HPP
