// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <yaml_config/fkyaml.h>

import std;
import nom.yaml_config;

TEST_CASE("C++ value codec agrees with Python corpus")
{
    const auto* input = std::getenv("YCFG_VALUE_INPUT");
    const auto* output = std::getenv("YCFG_VALUE_OUTPUT");
    if (!input || !output)
        return;
    std::ifstream stream(input, std::ios::binary);
    REQUIRE(stream.good());
    const std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    const auto node = nom::ycfg::parse_config(text, input);
    std::ofstream destination(output, std::ios::binary);
    destination << nom::ycfg::format_config(node);
    REQUIRE(destination.good());
}

TEST_CASE("Integer boundaries parse without signed overflow")
{
    using nom::ycfg::parse_config;
    CHECK(parse_config("-9223372036854775808").as_int() == std::numeric_limits<std::int64_t>::min());
    for (const auto text : {"18446744073709551615", "0xFFFFFFFFFFFFFFFF", "0o1777777777777777777777"})
        CHECK(parse_config(text).as_uint() == std::numeric_limits<std::uint64_t>::max());
    CHECK(parse_config("0x8000000000000000").as_uint() == (std::uint64_t{1} << 63));
    CHECK(parse_config("0o1000000000000000000000").as_uint() == (std::uint64_t{1} << 63));
    CHECK(parse_config("0x10000000000000000").is_string());
    CHECK(parse_config("0o2000000000000000000000").is_string());
}

TEST_CASE("Floating emission preserves precision and ignores the process locale")
{
    struct comma_decimal : std::numpunct<char>
    {
        char do_decimal_point() const override
        {
            return ',';
        }
    };
    struct locale_guard
    {
        std::locale previous = std::locale();
        ~locale_guard()
        {
            std::locale::global(previous);
        }
    } guard;
    std::locale::global(std::locale(std::locale::classic(), new comma_decimal));
    for (const double value :
         {1.2345678901234567, -0.0, std::numeric_limits<double>::denorm_min(), std::numeric_limits<double>::max()})
    {
        const auto text = nom::ycfg::format_config(fkyaml::node(value));
        const auto actual = nom::ycfg::parse_config(text).as_float();
        CHECK(actual == value);
        CHECK(std::signbit(actual) == std::signbit(value));
    }
}
