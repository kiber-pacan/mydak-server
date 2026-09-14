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

#include "parameters_base.hpp"

namespace mydak::args {
    namespace details {
        constexpr uint8_t TYPE_SMALL_NUMBER = 0;
        constexpr uint8_t TYPE_STRING = 1;
        constexpr uint8_t TYPE_IP = 2;
    }

    #pragma region Utils
    // Helper method for easier understanding of things
    template <std::size_t Index, std::size_t Option_String_Size, typename... Args>
    struct raw_parameter {
        constexpr raw_parameter(
            std::integral_constant<std::size_t, Index> type_index,
            tools::static_string<Option_String_Size> option, std::tuple<Args...> args
        ) : type_index(type_index), option(option), args(args) {}

        std::integral_constant<std::size_t, Index> type_index;
        tools::static_string<Option_String_Size> option;
        std::tuple<Args...> args;
    };
    template <std::size_t N, std::size_t Index, typename... Args>
    raw_parameter(
        tools::static_string<N> option,
        std::tuple<Args...> args,
        std::integral_constant<std::size_t, Index> type_index
        ) -> raw_parameter<N, Index, Args...>;

    // IS_VALID_PAIR START
    template <typename... Args>
    struct is_valid_raw_parameter : std::false_type {};

    template <std::size_t Index, std::size_t Option_String_Size, typename... Args>
    struct is_valid_raw_parameter<raw_parameter<Index, Option_String_Size, Args...>>
        : std::true_type {};
    // IS_VALID_PAIR END
    #pragma endregion



    #pragma region Variants
    // ReSharper disable once CppFunctionIsNotImplemented; I fucking hate yellow highlights
    template <std::size_t... Indices>
    constexpr auto make_parameters_from_indices(std::index_sequence<Indices...>) -> std::variant<parameter<Indices>...>;

    template <std::size_t N>
    using make_parameter_variants = decltype(make_parameters_from_indices(std::make_index_sequence<N>()));


    // VARIANT COUNT
    struct parameter_variants : make_parameter_variants<parameters_variant_count> {
        using variant::variant;
    };
    #pragma endregion


    #pragma region Make parameters


    /**
     * @brief Helper function for cleaner syntax of the 'make_parameters' function.
     *
     * @tparam N Index of the parameter and the std::variant. should be less than variants_count.
     *
     * @tparam Option Name of the command e.g. --db-hostname.
     *
     * @param args Parameters for the initialization of parameter<N>.
     *
     * @return A pair containing integral_constant index and the pair with the Option and the tuple of arguments.
     */
    template <tools::static_string Option, std::size_t N, typename... Args>
    requires (N < parameters_variant_count)
    constexpr auto make_parameter(Args... args) {
        return raw_parameter(std::integral_constant<std::size_t, N>{}, Option, std::make_tuple(args...));
    }

    template <typename... Args, std::size_t... Indices>
    constexpr auto make_from_indices(
        const std::tuple<Args...>& tuple,
        std::index_sequence<Indices...>
    ) {
        return parameter(std::get<Indices>(tuple)...);
    }

    template <typename... Args>
    constexpr auto make_from_tuple(
        const std::tuple<Args...>& tuple
    ) {
        return make_from_indices(tuple, std::make_index_sequence<sizeof...(Args)>());
    }

    template <typename Raw_Paramater>
    void test(Raw_Paramater p) {
        make_from_tuple(p.args);
    }

    /**
     * @brief Returns a parameter_variants array of the parameters and corresponding variant indices.
     *
     * Function for getting constexpr parameter_variants arrays of the parameters,
     * also as a bonus it provides std::variant indices corresponding to the parameters.
     *
     *
     * @param raw_parameters
     * std::pair<
     *     std::integral_constant<
     *         std::size_t, {Parameter type index}
     *     >,
     *
     *     std::pair <
     *         {Option name (static_string)},
     *         std::tuple<
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
    template <typename... Raw_Parameters>
    constexpr auto make_parameters(Raw_Parameters&&... raw_parameters)
    requires (!std::is_lvalue_reference_v<Raw_Parameters> && ...) // All pairs should be rvalues
    {
        static_assert((is_valid_raw_parameter<Raw_Parameters>::value && ...),
            "Provided type is not std::pair<std::integral_constant<std::size_t, N>, std::tuple<Args...>>!");

        // Creating new type with indices from Pairs
        using type_indices = std::index_sequence<raw_parameters.type_index...>;
        constexpr type_indices type_index{}; // e.g 0 | 1 | 2

        auto parameters = std::array<parameter_variants, sizeof...(raw_parameters)>{
           parameter_variants(make_from_tuple(raw_parameters.args))...
        };
        parameter_variants(make_from_tuple(raw_parameters...[0].args));
        auto options = std::make_tuple(raw_parameters.option...);


        // returning new array and empty type_indices object
        return std::make_tuple(parameters, options, type_index);
    }
    #pragma endregion


    #pragma region Setup
    static constexpr auto tuple_boy = make_parameters(
        make_parameter<"--db-hostname", details::TYPE_IP>("localhost"),
        make_parameter<"--db-username", details::TYPE_STRING>(4, 64, "username"),
        make_parameter<"--db-password", details::TYPE_STRING>(4, 64, "password")
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

    [[nodiscard]] consteval auto get_parameters() {return parameters;}
    [[nodiscard]] consteval auto get_options_tuple() {return options_tuple;}
    [[nodiscard]] consteval auto get_type_sequence() {return type_sequence;}
    #pragma endregion

    void help();
}

#endif //MYDAK_BACKEND_PARAMS_H
