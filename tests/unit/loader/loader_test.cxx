// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
#include <catch2/catch_test_macros.hpp>
#include <yaml_config/config.h>
import std;
import nom.yaml_config;

TEST_CASE("runtime substitution is recursive single pass and preserves keys and scalar types")
{
    auto doc = nom::ycfg::parse_config(R"(
'{ycfg:run-var KEY}': 'unchanged'
array: ['prefix {ycfg:run-var A}', '{ycfg:run-var EMPTY}']
nested: { value: '{ycfg:run-var A}' }
integer: 20
)");
    auto lookup = [](std::string_view name) -> std::optional<std::string>
    {
        if (name == "A")
            return "{ycfg:run-var OTHER}";
        if (name == "EMPTY")
            return "";
        return {};
    };
    nom::ycfg::substitute_runtime_variables(doc, lookup);
    CHECK(doc.at("{ycfg:run-var KEY}").as_str() == "unchanged");
    CHECK(doc.at("array").at(0).as_str() == "prefix {ycfg:run-var OTHER}");
    CHECK(doc.at("array").at(1).as_str().empty());
    CHECK(doc.at("nested").at("value").as_str() == "{ycfg:run-var OTHER}");
    CHECK(doc.at("integer").is_integer());
    for (const std::string value :
         {"{ycfg:run-var}", "{ycfg:run-var X }", "{ycfg:run-var X", "{ycfg:run-var }", "{ycfg:run-var MISSING}"})
    {
        fkyaml::node candidate(fkyaml::node::mapping_type{{"a/b~c", fkyaml::node::sequence_type{fkyaml::node(value)}}});
        try
        {
            nom::ycfg::substitute_runtime_variables(candidate, lookup);
            FAIL("expected substitution failure");
        }
        catch (const nom::ycfg::config_error& e)
        {
            CHECK(e.stage == nom::ycfg::config_stage::substitution);
            CHECK(e.pointer == "/a~1b~0c/0");
        }
    }
}

namespace override_fixture
{
    struct config
    {
        nom::cfg_int count;
        nom::cfg_string text;
        nom::cfg_vector<nom::cfg_int> items;
        nom::cfg_map<nom::cfg_int> named;
    };
    NOM_YCONFIG(config, count, text, items, named)
} // namespace override_fixture
TEST_CASE("overrides are string first then typed YAML and only replace existing targets")
{
    auto doc = nom::ycfg::parse_config("count: 1\ntext: 'one'\nitems: [1, 2]\nnamed: { 'a/b~c': 3 }");
    auto probe = [](const fkyaml::node& value) { (void)value.get_value<override_fixture::config>(); };
    const std::vector<nom::ycfg::config_override> values{
        {"/text", "20"}, {"/count", "0x20"}, {"/items/1", "4"}, {"/named/a~1b~0c", "5"}};
    nom::ycfg::apply_config_overrides(doc, values, probe);
    CHECK(doc.at("text").as_str() == "20");
    CHECK(doc.at("count").as_int() == 32);
    CHECK(doc.at("items").at(1).as_int() == 4);
    CHECK(doc.at("named").at("a/b~c").as_int() == 5);
    for (const auto& [pointer, value] : std::vector<std::pair<std::string, std::string>>{{"/count", "1.0"},
                                                                                         {"/count", "1\nextra: 2"},
                                                                                         {"/missing", "1"},
                                                                                         {"/items/-", "1"},
                                                                                         {"/items/01", "1"},
                                                                                         {"/items/+1", "1"},
                                                                                         {"/items/2", "1"},
                                                                                         {"/items/", "1"},
                                                                                         {"/named/a~2b", "1"},
                                                                                         {"count", "1"},
                                                                                         {"/count/subkey", "1"},
                                                                                         {"/count", "null"}})
    {
        CAPTURE(pointer, value);
        const auto original = doc;
        const std::array changes{nom::ycfg::config_override{pointer, value}};
        REQUIRE_THROWS_AS(nom::ycfg::apply_config_overrides(doc, changes, probe), nom::ycfg::config_error);
        CHECK(doc == original);
    }
    const std::array root{nom::ycfg::config_override{"", "{ count: 9, text: 'root', items: [], named: {} }"}};
    nom::ycfg::apply_config_overrides(doc, root, probe);
    CHECK(doc.at("count").as_int() == 9);
}

TEST_CASE("runtime environment and string overrides reject invalid UTF-8")
{
    auto document = nom::ycfg::parse_config("value: '{ycfg:run-var INVALID}'");
    try
    {
        nom::ycfg::substitute_runtime_variables(document,
                                                [](std::string_view) -> std::optional<std::string> { return "\xff"; });
        FAIL("invalid environment text accepted");
    }
    catch (const nom::ycfg::config_error& e)
    {
        CHECK(e.stage == nom::ycfg::config_stage::substitution);
        CHECK(e.pointer == "/value");
    }
    document["value"] = "valid";
    const std::array changes{nom::ycfg::config_override{"/value", "\xff"}};
    REQUIRE_THROWS_AS(nom::ycfg::apply_config_overrides(document, changes, [](const fkyaml::node&) {}),
                      nom::ycfg::config_error);
    CHECK(document.at("value").as_str() == "valid");
}
