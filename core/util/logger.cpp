#include "logger.hpp"



namespace mydak {
	void call_log(bool error, bool debug, bool func, std::string_view message, std::string_view function) {
		if (debug and !DEBUG) return;

		std::ostream& out = error ? std::cerr : std::cout;
		std::string_view separator = !message.empty() and func ? " : " : ""; 
		auto index0 = function.find('(');
		auto index1 = function.find(' ');
		std::string_view function_name =
			func ?
			(index0 != std::string_view::npos ?
				(index1 != std::string_view::npos ?
					function.substr(index1 + 1, index0 - index1 - 1)
					: function.substr(0, index0)
				)
				: function
			)
			:
			""; 
		
		out << function_name << separator << message << '\n';
	}

	
	void log(
		std::string_view message,
		std::source_location source
	) {
		call_log(false, false, false, message, source.function_name());
	}

	void log_error(
		std::string_view message,
		std::source_location source
	) {
		call_log(false, true, false, message, source.function_name());
	}

	void log_debug(
		std::string_view message,
		std::source_location source
	) {
		call_log(false, true, false, message, source.function_name());
	}
	void log_debug_error(
		std::string_view message,
		std::source_location source
	) {
		call_log(true, true, false, message, source.function_name());
	}


	
	void log_func(
		std::string_view message,
		std::source_location source
	) {
		call_log(false, false, true, message, source.function_name());
	}

	void log_func_error(
		std::string_view message,
		std::source_location source
	) {
		call_log(false, true, true, message, source.function_name());
	}

	void log_func_debug(
		std::string_view message,
		std::source_location source
	) {
		call_log(false, true, true, message, source.function_name());
	}
	void log_func_debug_error(
		std::string_view message,
		std::source_location source
	) {
		call_log(true, true, true, message, source.function_name());
	}
}
