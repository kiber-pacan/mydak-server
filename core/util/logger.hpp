#ifndef MYDAK_BACKEND_CORE_UTIL_LOGGER_HPP
#define MYDAK_BACKEND_CORE_UTIL_LOGGER_HPP

#include <source_location>
#include <string_view>
#include <iostream>

#ifndef NDEBUG
inline constexpr bool DEBUG = true;
#else
inline constexpr bool DEBUG = false;
#endif
namespace boost::system {
	struct system_error;
}

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


	void exit(
		std::string_view message
	);

	void exit(
		const boost::system::system_error& e
	);

	void exit_func(
		std::string_view message,
		std::source_location source = std::source_location::current()
	);

	void exit_func(
		const boost::system::system_error& e,
		std::source_location source = std::source_location::current()
	);


	void exception(
		const boost::system::system_error& e
	);

	void exception_func(
		const boost::system::system_error& e,
		std::source_location source = std::source_location::current()
	);
}
#endif  // MYDAK_BACKEND_CORE_UTIL_LOGGER_HPP
