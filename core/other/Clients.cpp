#ifndef MYDAK_WEBSOCKET_CORE_INITIALIZER_HPP
#define MYDAK_WEBSOCKET_CORE_INITIALIZER_HPP

#include <filesystem>
#include <fstream>
#include <iostream>

#include <toml++/toml.hpp>
#include <map>

struct Clients {
	Clients() {
		auto path = std::filesystem::current_path();
		dataPath = path / "data";

		std::filesystem::create_directories(dataPath);

		toml::array array("ratelimited");
		std::ofstream clients(dataPath / "clients.toml");	
	}

	void addClient(std::string publicKey, std::string ip) {
		toml::table tbl;
		try {
			auto table = toml::parse_file((dataPath / "clients.toml").c_str());

			table.insert(publicKey, ip);
			std::ofstream clients(dataPath / "clients.toml");
			clients << table;
		}
		catch (const toml::parse_error& err) {
			std::cerr << "Parsing failed:\n" << err << "\n";
		}
	}

	void addRateLimited(std::string ip) {
		toml::table tbl;
		try {
			auto table = toml::parse_file((dataPath / "clients.toml").c_str());
			
			std::ofstream clients(dataPath / "clients.toml");
			clients << table;
		}
		catch (const toml::parse_error& err) {
			std::cerr << "Parsing failed:\n" << err << "\n";
		}
	}

private:
	std::filesystem::path dataPath;
};

#endif  // MYDAK_WEBSOCKET_CORE_INITIALIZER_HPP
