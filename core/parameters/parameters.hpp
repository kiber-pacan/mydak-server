//
// Created by akicatt on 31.07.2026.
//

#ifndef MYDAK_BACKEND_PARAMS_H
#define MYDAK_BACKEND_PARAMS_H
#include <algorithm>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "database/database.hpp"
#include "boost/asio.hpp"
#include "util/logger.hpp"


namespace mydak::args {
    inline bool is_a_number(const std::string_view text) {
        return !text.empty() ? std::ranges::all_of(text, [](auto& character){ return std::isdigit(character); }) : false;
    }


    inline bool is_an_ip(const std::string& raw_ip) {
        if (raw_ip == "localhost") {
            logger::log_debug("localhost is discouraged from using!");
            return true;
        }
        boost::system::error_code error;
        boost::asio::ip::make_address(raw_ip, error);
        return error ? false : true;
    }

    // PARAMETER BASE START
    template <typename T>
    struct parameter_base {
        explicit parameter_base() = default;
        parameter_base(const int&& min, const int&& max, T default_value) : data(default_value), min(min), max(max) {}
        virtual ~parameter_base() = default;


        virtual void try_set_val(const std::string& value) = 0;

        [[nodiscard]] virtual std::string to_string() const = 0;

        [[nodiscard]] std::string limits_to_string() const {
            return std::format("from {} to {}", min, max);
        }

        [[nodiscard]] T get_data() const {
            return data;
        }

    protected:
        [[nodiscard]] virtual bool is_in_limits(const T& value) const = 0;

        T data;
        int32_t min{};
        int32_t max{};
    };
    // PARAMETER BASE END

    #pragma region Parameter base definitions
    template <typename T>
    requires std::is_arithmetic_v<T>
    struct parameter_base_arithmetic : parameter_base<T> {
        using parameter_base<T>::parameter_base;
        static constexpr uint8_t type_val = 0;

        void try_set_val(const std::string& value) override {
            if (is_a_number(value)) {
                const auto number = static_cast<int8_t>(std::stoi(value));
                if (is_in_limits(number)) {
                   this->data = number;
                } else {
                    logger::exception(std::format("{} is not in bounds: {}!", value, this->limits_to_string()));
                }
            } else {
                logger::exception(std::format("{} is not a number!", value));
            }
        }

        [[nodiscard]] std::string to_string() const override {
            return std::to_string(this->data);
        }
    protected:
        [[nodiscard]] bool is_in_limits(const T& num) const override {
            return num >= this->min && num <= this->max;
        }
    };


    struct parameter_base_string : parameter_base<std::string>  {
        using parameter_base::parameter_base;
        static constexpr uint8_t type_val = 1;

        void try_set_val(const std::string& value) override {
            if (is_in_limits(value)) {
                data = value;
            } else {
                logger::exception(std::format("{} is not in bounds: {}!", value, limits_to_string()));
            }
        }

        [[nodiscard]] std::string to_string() const override {
            return data;
        }

    protected:
        [[nodiscard]] bool is_in_limits(const std::string& string) const override {
            return string.size() >= min && string.size() <= max;
        }
    };
    #pragma endregion

    template <uint8_t type>
    struct parameter;

    // int_8t - 0
    template<>
    struct parameter<0> : parameter_base_arithmetic<int8_t> {
        using parameter_base_arithmetic::parameter_base_arithmetic;
    };

    // Basic string - 1
    template<>
    struct parameter<1> : parameter_base_string {
        using parameter_base_string::parameter_base_string;
    };

    // IP - 2
    template<>
    struct parameter<2> : parameter_base_string {
        parameter(const std::string& hostname) {
            this->data = hostname;
            this->min = 0;
            this->max = 1024;
        }

        using parameter_base_string::parameter_base_string;
        void try_set_val(const std::string& value) override {
            if (is_in_limits(value)) {
                if (is_an_ip(value)) {
                    data = value;
                } else {
                    logger::exception(std::format("{} is not an ip", value));
                }
            } else {
                logger::exception(std::format("{} is not in bounds: {}!", value, limits_to_string()));
            }
        }
    };
    // PARAMETER TYPES END

    using parameter_variant = std::variant<parameter<0>, parameter<1>, parameter<2>>;


    void help();

    [[nodiscard]] std::vector<parameter_variant> process_args(const int argc, char* argv[]);
}

#endif //MYDAK_BACKEND_PARAMS_H
