// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>
#include <yaml_config/config.h>
import std;
import nom.yaml_config;

template <typename T> T get(const fkyaml::node& node)
{
    return node.get_value<T>();
}

namespace fixture
{
    using namespace nom::cfg_types;
    enum class level
    {
        debug,
        info,
        warn,
        invalid
    };
    NOM_YCONFIG_ENUM(level,
                     {{level::debug, "debug"}, {level::info, "info"}, {level::warn, "warn"}, {level::info, "INFO"}})
    struct connection
    {
        cfg_string address;
        cfg_defaulted<cfg_int> timeout{5};
        cfg_optional<cfg_string> interface;
    };
    NOM_YCONFIG(connection, address, timeout, interface)
    struct settings
    {
        connection required{{}, {42}, "loopback"};
        cfg_defaulted<connection> defaults{{"localhost", {75}, {}}};
        cfg_optional<connection> optional{connection{"optional", {90}, "eth0"}};
        cfg_defaulted<level> severity{level::info};
        cfg_defaulted<cfg_vector<level>> levels;
        cfg_defaulted<cfg_map<level>> named;
        cfg_defaulted<cfg_unordered_map<cfg_int>> numbers;
        cfg_defaulted<cfg_vector<cfg_bool>> flags;
    };
    NOM_YCONFIG(settings, required, defaults, optional, severity, levels, named, numbers, flags)
    struct empty
    {
    };
    NOM_YCONFIG(empty)
    struct native
    {
        cfg_int count;
    };
    struct custom
    {
        std::string value;
    };
    void from_node(const fkyaml::node& v, custom& c)
    {
        c.value = v.as_str();
    }
    void to_node(fkyaml::node& v, const custom& c)
    {
        v = c.value;
    }
    struct native_custom
    {
        std::string value;
    };
} // namespace fixture
namespace fixture
{
    void from_node(const fkyaml::node& node, native& result)
    {
        node.at("count").get_value_inplace(result.count);
    }
    void to_node(fkyaml::node& node, const native& value)
    {
        node = fkyaml::node::mapping();
        node["count"] = value.count;
    }
    void from_node(const fkyaml::node& node, native_custom& result)
    {
        result.value = node.as_str();
    }
    void to_node(fkyaml::node& node, const native_custom& result)
    {
        node = result.value;
    }
} // namespace fixture
template <> inline constexpr bool nom::enable_config_type<fixture::custom> = true;
template <> inline constexpr bool nom::enable_config_type<fixture::native_custom> = true;
namespace fixture
{
    struct move_only
    {
        std::string text;
        move_only() = default;
        move_only(move_only&&) = default;
        move_only& operator=(move_only&&) = default;
        move_only(const move_only&) = delete;
        move_only& operator=(const move_only&) = delete;
    };
    NOM_YCONFIG(move_only, text)
    struct move_only_list
    {
        cfg_vector<move_only> items;
    };
    NOM_YCONFIG(move_only_list, items)
    struct domains
    {
        custom a;
        native_custom b;
    };
    NOM_YCONFIG(domains, a, b)
} // namespace fixture
static_assert(!nom::is_config_member<int>);
static_assert(!nom::is_config_member<std::vector<std::optional<nom::cfg_int>>>);
static_assert(!nom::is_config_member<nom::cfg_defaulted<std::optional<nom::cfg_int>>>);

TEST_CASE("nested member policies preserve initialized defaults")
{
    const auto doc = nom::ycfg::parse_config("required: { address: 'remote' }\noptional: { address: 'changed' }");
    auto value = get<fixture::settings>(doc);
    REQUIRE(*value.required.timeout == 42);
    REQUIRE(value.required.interface == "loopback");
    REQUIRE(*value.defaults->timeout == 75);
    REQUIRE(*value.optional->timeout == 90);
    REQUIRE(value.optional->interface == "eth0");
    value.required.timeout = {123};
    nom::ycfg::detail::read_value(doc, value);
    REQUIRE(*value.required.timeout == 123);
    REQUIRE_THROWS(get<fixture::settings>(nom::ycfg::parse_config("")));
    REQUIRE_THROWS(get<fixture::settings>(nom::ycfg::parse_config("required: {}")));
    REQUIRE_THROWS(get<fixture::settings>(fkyaml::node(fkyaml::node::sequence_type{})));
    REQUIRE_NOTHROW(get<fixture::empty>(nom::ycfg::parse_config("ignored: 1")));
    REQUIRE_THROWS(get<fixture::empty>(fkyaml::node(42)));
}

TEST_CASE("serialization omits empty optionals and round trips active values")
{
    auto value = get<fixture::settings>(nom::ycfg::parse_config("required: { address: 'remote' }"));
    value.optional.reset();
    value.required.interface.reset();
    const fkyaml::node serialized(value);
    REQUIRE_FALSE(serialized.contains("optional"));
    REQUIRE_FALSE(serialized.at("required").contains("interface"));
    REQUIRE(serialized.at("required").at("timeout").as_int() == 42);
    REQUIRE(serialized.at("severity").as_str() == "info");
    const auto restored = get<fixture::settings>(serialized);
    REQUIRE(restored.optional->address == "optional");
    REQUIRE(restored.required.interface == "loopback");
}

TEST_CASE("checked numeric conversions reject lossy or unsupported values")
{
    for (const fkyaml::node& v :
         {fkyaml::node(1.0), fkyaml::node(true), fkyaml::node("1"), fkyaml::node(fkyaml::node::sequence_type{})})
    {
        REQUIRE_THROWS(get<nom::cfg_int>(v));
        REQUIRE_THROWS(get<nom::cfg_uint>(v));
    }
    REQUIRE(get<nom::cfg_int>(fkyaml::node(std::numeric_limits<std::int64_t>::min())).value ==
            std::numeric_limits<std::int64_t>::min());
    REQUIRE(get<nom::cfg_uint>(fkyaml::node(std::numeric_limits<std::int64_t>::max())).value ==
            std::numeric_limits<std::int64_t>::max());
    REQUIRE_THROWS(get<nom::cfg_uint>(fkyaml::node(-1)));
    REQUIRE(fkyaml::node(nom::cfg_uint{std::numeric_limits<std::uint64_t>::max()}).as_uint() ==
            std::numeric_limits<std::uint64_t>::max());
    REQUIRE_THROWS(get<nom::cfg_int>(fkyaml::node(std::numeric_limits<std::uint64_t>::max())));
    REQUIRE(get<nom::cfg_double>(fkyaml::node(2)).value == 2.0);
    for (double v : {std::numeric_limits<double>::infinity(),
                     -std::numeric_limits<double>::infinity(),
                     std::numeric_limits<double>::quiet_NaN()})
    {
        REQUIRE_THROWS(get<nom::cfg_double>(fkyaml::node(v)));
        REQUIRE_THROWS(fkyaml::node(nom::cfg_double{v}));
    }
    REQUIRE_THROWS(get<nom::cfg_double>(fkyaml::node(true)));
}

TEST_CASE("native ADL and enabled custom converters compose")
{
    REQUIRE(get<fixture::native>(nom::ycfg::parse_config("count: 2")).count == 2);
    REQUIRE_THROWS(get<fixture::native>(nom::ycfg::parse_config("count: 2.0")));
    const auto doc = nom::ycfg::parse_config("a: 'one'\nb: 'two'");
    auto result = get<fixture::domains>(doc);
    REQUIRE(result.a.value == "one");
    REQUIRE(result.b.value == "two");
    REQUIRE(fkyaml::node(result) == doc);
}

TEST_CASE("enum aliases canonicalize and failures leave the enum unchanged")
{
    using fixture::level;
    REQUIRE(get<level>(fkyaml::node("INFO")) == level::info);
    REQUIRE(fkyaml::node(level::info).as_str() == "info");
    for (const auto& v : {fkyaml::node("Info"),
                          fkyaml::node("unknown"),
                          fkyaml::node(1),
                          fkyaml::node(1.0),
                          fkyaml::node(true),
                          fkyaml::node(fkyaml::node::sequence_type{}),
                          fkyaml::node(fkyaml::node::mapping_type{}),
                          fkyaml::node()})
    {
        auto result = level::warn;
        REQUIRE_THROWS(nom::ycfg::detail::read_value(v, result));
        REQUIRE(result == level::warn);
    }
    fkyaml::node output("unchanged");
    REQUIRE_THROWS(fixture::to_node(output, level::invalid));
    REQUIRE(output.as_str() == "unchanged");
    REQUIRE_THROWS(fkyaml::node(static_cast<level>(100)));
}

TEST_CASE("containers retain nested error pointers and replace their contents")
{
    for (const auto& [text, pointer] :
         std::vector<std::pair<std::string, std::string>>{{"severity: 'typo'", "/severity"},
                                                          {"levels: ['info', 'typo']", "/levels/1"},
                                                          {"named: { 'a/b~c': 'typo' }", "/named/a~1b~0c"},
                                                          {"optional: {address: 10}", "/optional/address"}})
    {
        const auto doc = nom::ycfg::parse_config("required: { address: 'remote' }\n" + text);
        try
        {
            (void)get<fixture::settings>(doc);
            FAIL("expected conversion failure");
        }
        catch (const nom::ycfg::mapping_error& e)
        {
            REQUIRE(e.pointer == pointer);
        }
    }
    const auto doc = nom::ycfg::parse_config("required: { address: 'remote' }\nlevels: ['warn']\nnamed: { a: "
                                             "'debug' }\nnumbers: { a: 4 }\nflags: [true, false]");
    auto result = get<fixture::settings>(doc);
    REQUIRE(result.levels->at(0) == fixture::level::warn);
    REQUIRE(result.named->at("a") == fixture::level::debug);
    REQUIRE(result.numbers->at("a") == 4);
    REQUIRE(result.flags->at(0));
    REQUIRE(fkyaml::node(result).at("flags").at(1).as_bool() == false);
    auto empty = doc;
    empty["levels"] = fkyaml::node::sequence_type{};
    nom::ycfg::detail::read_value(empty, result);
    REQUIRE(result.levels->empty());
}

TEST_CASE("container serialization does not require copyable config values")
{
    const auto document = nom::ycfg::parse_config("items: [{text: 'hello'}]");
    const auto value = get<fixture::move_only_list>(document);
    REQUIRE(value.items.front().text == "hello");
    REQUIRE(fkyaml::node(value) == document);
}

namespace fixture
{
    struct strict_scalars
    {
        cfg_bool flag;
        cfg_string name;
        cfg_vector<cfg_bool> flags;
        cfg_map<cfg_bool> switches;
    };
    NOM_YCONFIG(strict_scalars, flag, name, flags, switches)
} // namespace fixture

TEST_CASE("boolean categories stay strict through member and container helpers")
{
    const auto valid = nom::ycfg::parse_config("flag: true\nname: text\nflags: [true]\nswitches: {a: false}\n");
    for (const auto path : {"flag", "flags", "switches"})
        for (const auto& invalid : {fkyaml::node(1), fkyaml::node(0.0), fkyaml::node("true"), fkyaml::node()})
        {
            auto doc = valid;
            if (std::string_view(path) == "flag")
                doc[path] = invalid;
            else if (std::string_view(path) == "flags")
                doc[path][0] = invalid;
            else
                doc[path]["a"] = invalid;
            REQUIRE_THROWS(doc.get_value<fixture::strict_scalars>());
        }
    auto doc = valid;
    doc["name"] = 42;
    REQUIRE_THROWS(doc.get_value<fixture::strict_scalars>());
    REQUIRE(fkyaml::node(valid.get_value<fixture::strict_scalars>()) == valid);
}

TEST_CASE("full uint64 values round trip through native checked adapters")
{
    for (const std::uint64_t value :
         {std::uint64_t{0}, std::uint64_t{1} << 63, std::numeric_limits<std::uint64_t>::max()})
    {
        const fkyaml::node node = nom::cfg_uint{value};
        REQUIRE(node.get_value<nom::cfg_uint>().value == value);
        const auto restored = nom::ycfg::parse_config(nom::ycfg::format_config(node));
        REQUIRE(restored.get_value<nom::cfg_uint>().value == value);
        REQUIRE(std::isfinite(restored.get_value<nom::cfg_double>().value));
    }
}
