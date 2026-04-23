#pragma once

#include <charconv>
#include <concepts>
#include <exception>
#include <format>
#include <iterator>
#include <meta>
#include <optional>
#include <pthread.h>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace clap
{

class Exception : public std::runtime_error
{
  public:
    template <class... Args>
    constexpr Exception(std::format_string<Args...> msg, Args &&...args)
        : std::runtime_error{std::format(msg, std::forward<Args>(args)...)}
    {
    }
};

namespace impl
{

template <class T>
concept IsOptional = std::same_as<T, std::optional<std::ranges::range_value_t<T>>>;

constexpr auto convert_raw_args(const int argc, char const *const *argv) -> std::vector<std::string_view> //
    pre(argc > 0)                                                                                         //
    pre(argv != nullptr)                                                                                  //
    post(r : std::ranges::size(r) == argc - 1zu)
{
    return std::span{argv, argv + argc} |                                                 //
           std::views::drop(1) |                                                          //
           std::views::transform([](const auto *arg) { return std::string_view{arg}; }) | //
           std::ranges::to<std::vector>();
}

constexpr auto try_find_arg_index(const std::span<const std::string_view> args, const std::string_view arg)
    -> std::optional<std::size_t> //
    pre(!std::ranges::empty(arg))
{
    const auto arg_iter = std::ranges::find(args, arg);
    return arg_iter == std::ranges::cend(args)
               ? std::nullopt
               : std::make_optional(std::ranges::distance(std::ranges::cbegin(args), arg_iter));
}

constexpr auto format_member_as_arg(const std::string_view member_name) -> std::string //
    pre(!std::ranges::empty(member_name))
{
    auto formatted = std::string{};

    for (auto chr : member_name)
    {
        if (chr == '_')
        {
            formatted += '-';
            continue;
        }

        if (chr >= 'A' && chr <= 'Z')
        {
            formatted += '-';
            formatted += chr + 32;
            continue;
        }

        formatted += chr;
    }

    return std::format("--{}", formatted);
}

consteval auto format_short_name(std::meta::info) -> std::string_view
{
    return {};
}

template <class T>
    requires std::same_as<T, std::string>
constexpr auto convert_value(const std::string_view value) -> T //
    pre(!std::ranges::empty(value))                             //
    post(r : std::ranges::size(value) == std::ranges::size(r))
{
    return T{value};
}

template <class T>
    requires(std::integral<T> && !std::same_as<T, bool>)
constexpr auto convert_value(const std::string_view value) -> T //
    pre(!std::ranges::empty(value))
{
    auto res = T{};

    if (const auto ec = std::from_chars(value.data(), value.data() + value.size(), res); !ec)
    {
        throw Exception("failed to convert '{}' to integral type", value);
    }

    return res;
}

template <class T>
    requires IsOptional<T>
constexpr auto convert_value(const std::string_view value) -> T::value_type //
    pre(!std::ranges::empty(value))
{
    return convert_value<typename T::value_type>(value);
}

template <class T>
struct DebugType;

}

template <char S>
struct ShortName
{
    constexpr static auto letter = S;
};

template <class T>
concept IsShortName = std::same_as<std::remove_cvref_t<T>, ShortName<T::letter>>;

template <class T>
    requires std::is_default_constructible_v<T>
constexpr auto parse(int argc, char const *const *argv) -> T //
    pre(argc > 0)
{
    auto args = impl::convert_raw_args(argc, argv);
    constexpr auto ctx = std::meta::access_context::current();

    auto res = T{};

    template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
    {
        using MemberType = typename[:std::meta::type_of(member):];

        auto arg_str_short = std::optional<std::string>{};

        template for (constexpr auto annotation : std::define_static_array(std::meta::annotations_of(member)))
        {
            using AnnotationType = typename[:std::meta::type_of(annotation):];

            if constexpr (IsShortName<AnnotationType>)
            {
                constexpr auto letter = AnnotationType::letter;

                if (!!arg_str_short)
                {
                    throw Exception("cannot have multiple ShortName annotations");
                }

                arg_str_short = std::format("-{}", letter);
            }
        }

        const auto arg_str_short_index =
            arg_str_short.and_then([&](const auto &str) { return impl::try_find_arg_index(args, str); });

        const auto arg_str_long = impl::format_member_as_arg(std::meta::identifier_of(member));
        const auto arg_str_long_index = impl::try_find_arg_index(args, arg_str_long);

        if (!!arg_str_short_index && !!arg_str_long_index)
        {
            throw Exception("cannot have both {} {} in args", args[*arg_str_short_index], args[*arg_str_long_index]);
        }

        if (!!arg_str_short && !arg_str_short_index)
        {
            throw Exception("missing arg: {}", *arg_str_short);
        }

        const auto arg_str = arg_str_short_index ? std::string{args[*arg_str_short_index]} : arg_str_long;

        if constexpr (std::same_as<MemberType, bool>)
        {
            res.[:member:] = impl::try_find_arg_index(args, arg_str).has_value();
        }
        else
        {
            if (const auto arg_index = impl::try_find_arg_index(args, arg_str); arg_index)
            {
                if (*arg_index == std::ranges::size(args) - 1zu)
                {
                    throw Exception("missing value for arg: {}", arg_str);
                }

                const auto arg_value = args[*arg_index + 1zu];
                res.[:member:] = impl::convert_value<MemberType>(arg_value);
            }
            else
            {
                if constexpr (!std::meta::has_default_member_initializer(member) && !impl::IsOptional<MemberType>)
                {
                    throw Exception("missing arg: {}", arg_str);
                }
            }
        }
    }

    return res;
}

}
