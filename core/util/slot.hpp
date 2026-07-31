#ifndef MYDAK_WEBSOCKET_UTIL_SLOT_HPP
#define MYDAK_WEBSOCKET_UTIL_SLOT_HPP

#include <optional>
#include <iostream>
#include <string_view>
#include <format>
#include <utility>

#include "logger.hpp"

constexpr std::string_view ASSIGN_TO_NON_NULL_OBJECT =
	"Tried to assign non empty object new value! function still worked as intended, but previous value was overwritten!";


namespace mydak {
	template <typename T>
	struct slot {		
		slot(T object) : object(object), generation(1) {}
		~slot() {}
		
		void clear() {
			object.reset();
		}

		bool empty() {
			return !object.has_value();
		}

		
		T& get_slot_value() {
			return object.value();
		}

		size_t get_slot_generation() {
			return generation;
		}

		
		template <typename... Args>
		void set_value(Args&&... args) {
			if (!empty()) {
				mydak::log_debug_error(ASSIGN_TO_NON_NULL_OBJECT);
				this->clear();
			}
			
			this->object.emplace(std::forward<Args>(args)...);
			generation++;
		}
		
		private:
		std::optional<T> object{};
		size_t generation{};
	};
}
#endif  // MYDAK_WEBSOCKET_UTIL_SLOT_HPP
