#ifndef MYDAK_WEBSOCKET_UTIL_OPTIONAL_REF_HPP
#define MYDAK_WEBSOCKET_UTIL_OPTIONAL_REF_HPP

#include <optional>

namespace mydak {
	template<typename T>
	struct optional_ref {
		optional_ref() noexcept = default;
		optional_ref(std::nullopt_t) noexcept {};

		optional_ref(T& data) noexcept :
			object(std::optional<std::reference_wrapper<T>>(data)) {}

		
		
		bool has_value() noexcept {
			return object.has_value();
		}
		
		T& value() noexcept {
			return object.value().get();
		}		
	private:
		std::optional<std::reference_wrapper<T>> object;
	};
}

#endif  // MYDAK_WEBSOCKET_UTIL_OPTIONAL_REF_HPP
