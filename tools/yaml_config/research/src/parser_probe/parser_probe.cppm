// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

module;
#include <fkYAML/node.hpp>

export module nom.yaml_parser_probe;
import std;

export namespace probe
{
    enum class mode
    {
        fast,
        slow
    };

    inline void from_node(const fkyaml::node& node, mode& value)
    {
        const auto& name = node.as_str();
        if (name == "fast")
            value = mode::fast;
        else if (name == "slow")
            value = mode::slow;
        else
            throw std::runtime_error("unknown mode");
    }

    inline void to_node(fkyaml::node& node, const mode& value)
    {
        switch (value)
        {
        case mode::fast:
            node = "fast";
            return;
        case mode::slow:
            node = "slow";
            return;
        }
        throw std::runtime_error("unmapped mode");
    }

    struct config
    {
        std::int64_t timeout = 5;
        mode run = mode::slow;
    };

    inline void from_node(const fkyaml::node& node, config& value)
    {
        if (node.contains("timeout"))
            node.at("timeout").get_value_inplace(value.timeout);
        if (node.contains("run"))
            node.at("run").get_value_inplace(value.run);
    }

    inline void to_node(fkyaml::node& node, const config& value)
    {
        node = fkyaml::node::mapping();
        node["timeout"] = value.timeout;
        node["run"] = value.run;
    }
} // namespace probe
