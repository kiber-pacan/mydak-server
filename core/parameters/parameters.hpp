//
// Created by akicatt on 31.07.2026.
//

#ifndef MYDAK_BACKEND_PARAMS_H
#define MYDAK_BACKEND_PARAMS_H
#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <variant>

#include "tools.hpp"
#include "util/logger.hpp"
#include <charconv>


namespace mydak::args {
    namespace details {
        constexpr uint8_t TYPE_SMALL_NUMBER = 0;
        constexpr uint8_t TYPE_STRING = 1;
        constexpr uint8_t TYPE_IP = 2;
    }

    #pragma region Utils
    template <typename T>
    concept has_size = requires(T t) {
        { t.size() } -> std::convertible_to<std::size_t>;
    };



    // OTHER START
    bool is_a_number(std::string_view text);

    bool is_an_ip(std::string_view raw_ip);
    // OTHER END





    // IS_VALID_PAIR START
    template <typename... Args>
    struct is_valid_pair : std::false_type {};

    template <std::size_t N, std::size_t N1, typename... Args>
    struct is_valid_pair<std::pair<std::integral_constant<std::size_t, N>, std::pair<tools::static_string<N1>, std::tuple<Args...>>>>
        : std::true_type {};
    // IS_VALID_PAIR END
    #pragma endregion


    #pragma region Parameters stuff
    template <typename T, typename Numeric>
    struct parameter_base {
        constexpr explicit parameter_base() = default;
        constexpr parameter_base(const Numeric min, const Numeric max, const T& default_value) : data(default_value), min(min), max(max) {}
        constexpr ~parameter_base() = default;

        [[nodiscard]] std::string to_string() const {
            if constexpr (std::is_arithmetic_v<T>) {
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
    #pragma endregion


    #pragma region Variants
    // ReSharper disable once CppFunctionIsNotImplemented; I fucking hate yellow highlights
    template <std::size_t... Indices>
    constexpr auto make_parameters_from_indices(std::index_sequence<Indices...>) -> std::variant<parameter<Indices>...>;

    template <std::size_t N>
    using make_parameter_variants = decltype(make_parameters_from_indices(std::make_index_sequence<N>()));


    // VARIANT COUNT
    using parameter_variants = make_parameter_variants<parameters_variant_count>;
    #pragma endregion


    #pragma region Make parameters
    /**
     * @brief Helper function for cleaner syntax of the 'make_parameters' function.
     *
     * @tparam N Index of the parameter and the std::variant. should be less than variants_count.
     *
     * @tparam Command_name Name of the command e.g. --db-hostname.
     *
     * @param args Parameters for the initialization of parameter<N>.
     *
     * @return A pair containing integral_constant index and the tuple of arguments.
     */
    template <tools::static_string Option, std::size_t N, typename... T>
    requires (N < parameters_variant_count)
    constexpr auto make_parameter(T... args) {
        return std::make_pair(std::integral_constant<std::size_t, N>{}, std::make_pair(Option, std::make_tuple(args...)));
    }

    /**
     * @brief Returns a parameter_variants array of the parameters and corresponding variant indices.
     *
     * Function for getting constexpr parameter_variants arrays of the parameters,
     * also as a bonus it provides std::variant indices corresponding to the parameters.
     *
     *
     * @param pairs
     * std::pair<
     *     std::integral_constant<
     *         std::size_t, {Parameter type index}
     *     >,
     *     std::tuple<
     *         std::pair <
     *             {Command name (static_string)}
     *             {Parameters for initialization of the corresponding parameter}
     *         >
     *     >
     * >
     *
     *
     * @return A tuple containing parameters_array and std::index_sequence for the std::variant.
     *
     *
     * @note You should use 'make_parameter' for getting pairs.
     *       Also intended to be used with structured bindings:
     *       @code
     *       constexpr auto [parameters, type_indices] = make_parameters(...);
     *       @endcode
     */
    template <typename... Pairs>
    constexpr auto make_parameters(Pairs&&... pairs)
    requires (!std::is_lvalue_reference_v<Pairs> && ...) // All pairs should be rvalues
    {
        static_assert((is_valid_pair<Pairs>::value && ...),
            "Provided type is not std::pair<std::integral_constant<std::size_t, N>, std::tuple<Args...>>!");

        // Creating new type with indices from Pairs
        using type_indices = std::index_sequence<pairs.first...>;
        constexpr type_indices type_indices_obj{};

        auto parameters = std::array<parameter_variants, sizeof...(pairs)>{std::make_from_tuple<parameter<pairs.first>>(pairs.second.second)...};
        auto options = std::make_tuple(pairs.second.first...);


        // returning new array and empty type_indices object
        return std::make_tuple(parameters, options, type_indices_obj);
    }
    #pragma endregion


    #pragma region Setup

    static constexpr auto tuple_boy = make_parameters(
        make_parameter<"--db-hostname", details::TYPE_IP>("localhost"),
        make_parameter<"--db-username", details::TYPE_STRING>(4, 64, "username"),
        make_parameter<"--db-password", details::TYPE_STRING>(4, 64, "password"),
        make_parameter<"--db-passw2ord", details::TYPE_STRING>(4, 64, "password"),
        make_parameter<"--db-pass4word", details::TYPE_STRING>(4, 64, "password")
    );

    static constexpr std::size_t parameters_count = std::get<0>(tuple_boy).size();

    static constexpr std::array<parameter_variants, std::size(std::get<0>(tuple_boy))> parameters = std::get<0>(tuple_boy);
    static constexpr auto options_tuple = std::get<1>(tuple_boy); // std::tuple of various static strings like --db-password
    static constexpr auto type_sequence = std::get<2>(tuple_boy); // std::sequence of corresponding types
    inline std::vector<std::string> immortal_strings{}; // using this for storing parameter strings

    // Indices for each option inside the parameters array
    static constexpr tools::static_map<std::size(parameters), std::size_t> options_indices = std::apply([](auto&&... args) {
        return tools::index_static_map(args.c_str()...);
    }, options_tuple);
    #pragma endregion


    struct parameters_accessor {
        constexpr ~parameters_accessor() = default;
        constexpr parameters_accessor() = default;
        explicit constexpr parameters_accessor(std::array<parameter_variants, parameters_count> parameters_internal) : parameters_internal(parameters_internal) {}

        template <std::size_t N>
        auto get() const
        // requires (N < parameters_count) linter is mad bout dis
        {
            // Getting type from our magic constexpr type_indices
            using type = std::decay_t<decltype(type_sequence)>;

            return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
        }

        template <tools::static_string Option>
        auto get() const {
            using type = std::decay_t<decltype(type_sequence)>;
            const auto N = options_indices.consteval_at<Option>();

            return std::get<tools::at<N, type>::value>(parameters_internal[N]).get_data();
       }
    private:
        std::array<parameter_variants, parameters_count> parameters_internal;
    };


    #pragma region Other
    void help();

    [[nodiscard]] parameters_accessor process_args(int argc, char* argv[]);
    #pragma endregion
}

#endif //MYDAK_BACKEND_PARAMS_H
