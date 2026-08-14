//
// Created by down on 14.08.2026.
//

#ifndef MYDAK_SERVER_PARAMETERS_BASE_H
#define MYDAK_SERVER_PARAMETERS_BASE_H
#include <string>

#include "logger.hpp"
#include <format>

namespace mydak::args {
    template <typename T>
    concept has_size = requires(T t) {
        { t.size() } -> std::convertible_to<std::size_t>;
    };

    // OTHER START
    bool is_a_number(std::string_view text);

    bool is_an_ip(std::string_view raw_ip);
    // OTHER END

    template <typename T, typename Numeric>
    struct parameter_base {
        constexpr explicit parameter_base() = default;
        constexpr parameter_base(const Numeric min, const Numeric max, const T& default_value) : data(default_value), min(min), max(max) {}
        constexpr ~parameter_base() = default;

        [[nodiscard]] std::string to_string() const {
            if constexpr (std::is_arithmetic_v<T>) {
                const int log10 = std::log10(this->data);
                const std::size_t buf_size = std::abs(log10) + (log10 < 0);
                char buf[buf_size]{};


                return std::to_string(this->data);
            } else {
                return std::string(this->data);
            }
        }

        [[nodiscard]] std::string limits_to_string() const {
            return std::format("from {} to {}", min, max);
        }

        template <typename T1>
        constexpr void get_data(T1& ptr) const {
            if constexpr (std::is_assignable_v<T1, T>) {
                ptr = data;
            }
        }

        [[nodiscard]] constexpr T get_data() const {
            return data;
        }

        [[nodiscard]] bool is_in_limits(const T& num) const {
            if constexpr (has_size<T>) {
                return num.size() >= this->min && num.size() <= this->max;
            } else {
                return num >= this->min && num <= this->max;
            }
        }

        void try_set_val_internal(const T& value) {
            if (is_in_limits(value)) {
                data = value;
            } else {
                logger::exception_func(std::format("{} is not in bounds: {}!", value, limits_to_string()));
            }
        }
    protected:
        T data{};
        Numeric min{};
        Numeric max{};
    };


    #pragma region Base
    template <typename T>
    requires std::is_arithmetic_v<T>
    // Parameter base that can have only types with arithmetic operators
    struct parameter_base_arithmetic : parameter_base<T, T> {
        using parameter_base<T, T>::parameter_base;

        void try_set_val(const char* value) {
            if (is_a_number(value)) {
                T number{};
                auto [ptr, ec] = std::from_chars(value, value + std::strlen(value), number);
                if (ec != std::errc{}) logger::exception_func(std::format("{} is not a number!", value));

                this->try_set_val_internal(number);
            } else {
                logger::exception_func(std::format("{} is not a number!", value));
            }
        }
    };


    struct parameter_base_string : parameter_base<std::string_view, uint32_t>  {
        using parameter_base::parameter_base;
        void try_set_val(std::string_view value) { this->try_set_val_internal(std::string_view(value)); }
    };
    #pragma endregion


    #pragma region Types
    template <uint8_t Type>
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
        explicit constexpr parameter(const std::string_view hostname)
            : parameter_base_string(0, 1024, hostname) {}

        void try_set_val(std::string_view value) {
            if (is_an_ip(value)) {
                try_set_val_internal(value);
            } else {
                logger::exception_func(std::format("{} is not an ip", value));
            }
        }
    };
    constexpr std::size_t parameters_variant_count = 3;

    #pragma endregion
}
#endif //MYDAK_SERVER_PARAMETERS_BASE_H
