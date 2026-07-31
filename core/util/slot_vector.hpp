#ifndef MYDAK_WEBSOCKET_UTIL_SLOT_VECTOR_HPP
#define MYDAK_WEBSOCKET_UTIL_SLOT_VECTOR_HPP


#include <queue>
#include <vector>

#include "logger.hpp"
#include "slot.hpp"

namespace mydak {
	template <typename T>
	struct slot_vector {
		slot<T>& operator[](const size_t& i) {
			return clients[i];
		}

		template <typename... Args>
		size_t emplacInitial commite_back(Args&&... args) {
			if (!empty_slots.empty()) {
				const auto& index = empty_slots.front();
				empty_slots.pop();

				// Of course we can do just this degenarate create/copy shit but whyyyy?
				// clients[index] = T{args...};

				// Oh boy we mogging
				// std::forward<Args>(args)... allows using std::move with rvalue references
				// For more efficiency
				// Just emplace into firts empty index in queue
				clients[index].set_value(std::forward<Args>(args)...);
				log_debug(std::format("Added client at index: {} (at old slot)", index));
				
				return index;
			}

			clients.emplace_back(T(std::forward<Args>(args)...));
			log_debug(std::format("Added at index: {} (at new slot)", global_index));
				
			return global_index++;
		}

		void pop(const size_t& index) {
			clients[index].clear();
			empty_slots.emplace(index);
			
			log_debug(std::format("Popped client at index: {}", index));
		}
		

		// PROBABLY SHOULDNT USE 
		const std::vector<mydak::slot<T>>& get() {
			return clients;
		}
		
	private:
		std::vector<slot<T>> clients{};
		std::queue<size_t> empty_slots{};
		size_t global_index = 0;
	};

}

#endif  // MYDAK_WEBSOCKET_UTIL_SLOT_VECTOR_HPP
