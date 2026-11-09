// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <fkYAML/node.hpp>

import std;
import nom.yaml_parser_probe;

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition)
            throw std::runtime_error(std::string(message));
    }

    template <class Function> void require_failure(Function&& action)
    {
        try
        {
            action();
        }
        catch (const std::exception&)
        {
            return;
        }
        throw std::runtime_error("expected conversion failure");
    }
} // namespace

int main()
{
    using node = fkyaml::node;
    try
    {
        for (const std::string text : {"on",
                                       "yes",
                                       "012",
                                       "0XFF",
                                       "-0xFF",
                                       "1_000",
                                       "2026-11-09",
                                       "18446744073709551616",
                                       "-9223372036854775809",
                                       "1e999",
                                       "\"12\""})
            require(node::deserialize(text).is_string(), "expected a string: " + text);
        require(node::deserialize("true").as_bool(), "boolean");
        require(node::deserialize("0o12").as_int() == 10, "octal");
        require(node::deserialize("0xFF").as_int() == 255, "hexadecimal");
        require(node::deserialize("1e3").as_float() == 1000.0, "exponent");
        require(std::isinf(node::deserialize(".inf").as_float()), "infinity");
        require(std::isnan(node::deserialize(".nan").as_float()), "nan");
        require(node::deserialize("null").is_null(), "null");
        require(node::deserialize("key:\n").at("key").is_null(), "implicit null");
        require(node::deserialize_docs("").empty(), "empty stream");
        require(node::deserialize_docs("a: 1\n---\na: 2\n").size() == 2, "stream documents");
        require_failure([] { node::deserialize("a: 1\na: 2\n"); });
        require(node::deserialize("a: &ref [1]\nb: *ref\n").at("b").is_alias(), "alias metadata");
        require(node::deserialize("a: &ref [1]\n").at("a").has_anchor_name(), "anchor metadata");
        require(node::deserialize("a: !!str 12\n").at("a").has_tag_name(), "tag metadata");
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        const auto unsigned_node = node::deserialize("18446744073709551615");
        require(unsigned_node.as_uint() == maximum, "uint64 parse");
        require(node::deserialize(node::serialize(node(maximum))).as_uint() == maximum, "uint64 roundtrip");
        require_failure([&] { unsigned_node.as_int(); });
        const auto source = node::deserialize("run: fast\n");
        auto fresh = source.get_value<probe::config>();
        require(fresh.timeout == 5 && fresh.run == probe::mode::fast, "fresh config");
        probe::config existing{27, probe::mode::slow};
        source.get_value_inplace(existing);
        require(existing.timeout == 27 && existing.run == probe::mode::fast, "preserved initializer");
        const node serialized = existing;
        require(serialized.at("timeout").as_int() == 27, "structure serialization");
        require(serialized.at("run").as_str() == "fast", "enum serialization");
        auto run = probe::mode::slow;
        require_failure([&] { node::deserialize("unknown").get_value_inplace(run); });
        require(run == probe::mode::slow, "enum unchanged after failure");
        require_failure([] { const node invalid = static_cast<probe::mode>(123); });
        std::cout
            << "PASS: fkYAML scalar boundaries, ADL mappings, enum failures, defaults, uint64 and module imports\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
