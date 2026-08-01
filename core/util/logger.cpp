#include "logger.hpp"



namespace {
	std::string get_func_name(std::string_view func) {
		const auto index0 = func.find('(');
		const auto index1 = func.find(' ');

		const std::string_view func_name =
			index0 != std::string_view::npos ?
				(index1 != std::string_view::npos ?
					func.substr(index1 + 1, index0 - index1 - 1)
					: func.substr(0, index0)
				)
				:
				func;

		return std::string(func_name);
	}
}

static void call_log(const bool error, const bool debug, std::string_view message, std::string_view func) {
	if (debug and !DEBUG) return;

	std::ostream& out = error ? std::cerr : std::cout;
	const std::string separator_and_message = !message.empty() ? std::format(" : {}", message) : "";

	out << get_func_name(func) << separator_and_message << '\n';
}

static void call_log(const bool error, const bool debug, std::string_view message) {
	if (debug and !DEBUG) return;

	std::ostream& out = error ? std::cerr : std::cout;

	out << message << '\n';
}


void mydak::logger::log(
	const std::string_view message
) {
	call_log(false, false, message);
}

void mydak::logger::log_error(
	const std::string_view message
) {
	call_log(false, true, message);
}

void mydak::logger::log_debug(
	const std::string_view message
) {
	call_log(false, true, message);
}

void mydak::logger::log_debug_error(
	const std::string_view message
) {
	call_log(true, true, message);
}



void mydak::logger::log_func(
	const std::string_view message,
	const std::source_location source
) {
	call_log(false, false, message, source.function_name());
}

void mydak::logger::log_func_error(
	const std::string_view message,
	const std::source_location source
) {
	call_log(false, true, message, source.function_name());
}

void mydak::logger::log_func_debug(
	const std::string_view message,
	const std::source_location source
) {
	call_log(false, true, message, source.function_name());
}
void mydak::logger::log_func_debug_error(
	const std::string_view message,
	const std::source_location source
) {
	call_log(true, true, message, source.function_name());
}

void mydak::logger::exception(
	const std::string& message
) {
	std::cout << message << "\n";
	std::exit(1);
}

void mydak::logger::exception_func(
	const std::string& message,
	const std::source_location source
) {
	std::cout <<  std::format("{} : {}", get_func_name(source.function_name()), !message.empty() ? std::format(" : {}", message) : "") << "\n";
	std::exit(1);
}

