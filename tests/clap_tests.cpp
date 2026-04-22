#include <contracts>
#include <iostream>
#include <print>
#include <stacktrace>
#include <string>

#include <gtest/gtest.h>

#include "clap/clap.h"

void handle_contract_violation(std::contracts::contract_violation cv);
void handle_contract_violation(std::contracts::contract_violation cv)
{
    std::println(std::cerr, "{}:{} {}", cv.location().file_name(), cv.location().line(), cv.comment());
    std::println(std::cerr, "{}", std::stacktrace::current());
}

namespace
{
struct Args
{
    std::string colour;
    auto operator==(const Args &) const -> bool = default;
};
}

TEST(clap, single_string)
{
    const auto expected = Args{.colour = "red"};
    const auto args = std::vector{"./program", "--colour", "red"};

    ASSERT_EQ(clap::parse<Args>(args.size(), args.data()), expected);
}

TEST(clap, missing_arg)
{
    const auto expected = Args{.colour = "red"};
    const auto args = std::vector{"./program", "red"};

    ASSERT_THROW(clap::parse<Args>(args.size(), args.data()), clap::Exception);
}

TEST(clap, missing_arg_value)
{
    const auto expected = Args{.colour = "red"};
    const auto args = std::vector{"./program", "--colour"};

    ASSERT_THROW(clap::parse<Args>(args.size(), args.data()), clap::Exception);
}
