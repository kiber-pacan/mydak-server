#ifndef MYDAK_BACKEND_CORE_PROTO_HPP
#define MYDAK_BACKEND_CORE_PROTO_HPP

namespace mydak {
	// Mydak protocol
	struct proto {
		static constexpr size_t E2E_KEYS_L = 32;

		static constexpr size_t MESSAGE_SIZE_L = 4;
		
		static constexpr char GREETINGS_PREFIX = 0x67;
		static constexpr char GREETINGS_PREFIX_L = 1;
	};
}
#endif  // MYDAK_BACKEND_CORE_PROTO_HPP
