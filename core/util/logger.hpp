#ifndef MYDAK_BACKEND_CORE_UTIL_LOGGER_HPP
#define MYDAK_BACKEND_CORE_UTIL_LOGGER_HPP


#include <source_location>
#include <string_view>


#ifndef NDEBUG
inline constexpr bool DEBUG = true;
#else
inline constexpr bool DEBUG = false;
#endif


namespace mydak::logger {
	void log(
		std::string_view message
	);

	void log_error(
		std::string_view message
	);

	void log_debug(
		std::string_view message
	);

	void log_debug_error(
		std::string_view message
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

	void exception(
		const std::string&  message
	);

	void exception_func(
		const std::string&  message,
		std::source_location source = std::source_location::current()
	);
}
#endif  // MYDAK_BACKEND_CORE_UTIL_LOGGER_HPP
