#ifndef MYDAK_WEBSOCKET_UTIL_LOGGER_HPP
#define MYDAK_WEBSOCKET_UTIL_LOGGER_HPP


#include <source_location>
#include <string_view>
#include <iostream>


#ifndef NDEBUG
inline constexpr bool DEBUG = true;
#else
inline constexpr bool DEBUG = false;
#endif


namespace mydak {	
	void log(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);

	void log_error(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);

	void log_debug(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);
	
	void log_debug_error(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);

	

	void log_func(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);

	void log_func_error(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);

	void log_func_debug(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);
	
	void log_func_debug_error(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);
}
#endif  // MYDAK_WEBSOCKET_UTIL_LOGGER_HPP
