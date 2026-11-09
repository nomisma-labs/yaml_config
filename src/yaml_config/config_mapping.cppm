// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;
#include "fkyaml.h"

export module nom.yaml_config:mapping;
import std;
import nom.config_types;

import :values;

export namespace nom::ycfg::detail
{
    template <typename Value> void read_value(const fkyaml::node& document, Value& destination);
    template <typename Value> fkyaml::node write_value(const Value& source);
    template <typename T> inline constexpr bool is_vector = false;
    template <typename T, typename Allocator> inline constexpr bool is_vector<std::vector<T, Allocator>> = true;
    template <typename T> inline constexpr bool is_map = false;
    template <typename T, typename Compare, typename Allocator>
    inline constexpr bool is_map<std::map<std::string, T, Compare, Allocator>> = true;
    template <typename T, typename Hash, typename Equal, typename Allocator>
    inline constexpr bool is_map<std::unordered_map<std::string, T, Hash, Equal, Allocator>> = true;
} // namespace nom::ycfg::detail

export namespace nom::detail
{
    template <typename Value> void from_node(const fkyaml::node& document, config_number<Value>& destination)
    {
        if constexpr (std::same_as<Value, double>)
        {
            const double value = document.is_integer() ? (document.is_uint() ? static_cast<double>(document.as_uint())
                                                                             : static_cast<double>(document.as_int()))
                                                       : document.as_float();
            if (!std::isfinite(value))
                throw ycfg::mapping_error("expected a finite number");
            destination.value = value;
        }
        else
        {
            if (!document.is_integer())
                throw ycfg::mapping_error("expected an integer; got " + fkyaml::node::serialize(document));
            if constexpr (std::same_as<Value, std::uint64_t>)
            {
                if (!document.is_uint())
                    throw ycfg::mapping_error("expected a nonnegative integer");
                destination.value = document.as_uint();
            }
            else if (document.is_uint())
            {
                const auto value = document.as_uint();
                if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
                    throw ycfg::mapping_error("integer exceeds the signed 64-bit range");
                destination.value = static_cast<std::int64_t>(value);
            }
            else
                destination.value = document.as_int();
        }
    }
    template <typename Value> void to_node(fkyaml::node& document, const config_number<Value>& source)
    {
        if constexpr (std::same_as<Value, double>)
            if (!std::isfinite(source.value))
                throw ycfg::mapping_error("expected a finite number");
        document = source.value;
    }
} // namespace nom::detail
export namespace nom
{
    template <typename Value> void from_node(const fkyaml::node& document, cfg_defaulted<Value>& destination)
    {
        ycfg::detail::read_value(document, destination.value);
    }
    template <typename Value> void to_node(fkyaml::node& document, const cfg_defaulted<Value>& source)
    {
        document = ycfg::detail::write_value(source.value);
    }
} // namespace nom

export namespace nom::ycfg::detail
{
    template <typename Value> void read_value(const fkyaml::node& document, Value& destination)
    {
        static_assert(nom::is_config_value<Value>, "value type is not config-enabled");
        if constexpr (std::same_as<Value, bool>)
            destination = document.as_bool();
        else if constexpr (std::same_as<Value, std::string>)
            destination = document.as_str();
        else if constexpr (is_vector<Value>)
        {
            const auto& values = document.as_seq();
            Value result;
            result.reserve(values.size());
            for (std::size_t i = 0; i < values.size(); ++i)
                at_member(std::to_string(i),
                          [&]
                          {
                              typename Value::value_type value{};
                              read_value(values[i], value);
                              result.push_back(std::move(value));
                          });
            destination = std::move(result);
        }
        else if constexpr (is_map<Value>)
        {
            Value result;
            for (const auto& [key, item] : document.as_map())
                at_member(key.as_str(), [&] { read_value(item, result[key.as_str()]); });
            destination = std::move(result);
        }
        else
            document.get_value_inplace(destination);
    }

    template <typename Value> fkyaml::node write_value(const Value& source)
    {
        static_assert(nom::is_config_value<Value>, "value type is not config-enabled");
        if constexpr (is_vector<Value>)
        {
            fkyaml::node::sequence_type result;
            for (std::size_t i = 0; i < source.size(); ++i)
                result.push_back(at_member(std::to_string(i),
                                           [&]
                                           {
                                               const typename Value::value_type& value = source[i];
                                               return write_value(value);
                                           }));
            return fkyaml::node(std::move(result));
        }
        else if constexpr (is_map<Value>)
        {
            fkyaml::node::mapping_type result;
            for (const auto& [key, value] : source)
                result.emplace(key, at_member(key, [&] { return write_value(value); }));
            return fkyaml::node(std::move(result));
        }
        else
            return fkyaml::node(source);
    }
    template <typename Value> void read_member(const fkyaml::node& document, const char* name, Value& destination)
    {
        at_member(name, [&] { read_value(document.at(name), destination); });
    }
    template <typename Value>
    void read_member(const fkyaml::node& document, const char* name, cfg_defaulted<Value>& destination)
    {
        if (document.contains(name))
            read_member(document, name, destination.value);
    }
    template <typename Value>
    void read_member(const fkyaml::node& document, const char* name, std::optional<Value>& destination)
    {
        if (document.contains(name))
        {
            if (!destination)
                destination.emplace();
            read_member(document, name, *destination);
        }
    }
    template <typename Value> void write_member(fkyaml::node& document, const char* name, const Value& source)
    {
        document[name] = at_member(name, [&] { return write_value(source); });
    }
    template <typename Value>
    void write_member(fkyaml::node& document, const char* name, const std::optional<Value>& source)
    {
        if (source)
            write_member(document, name, *source);
    }
} // namespace nom::ycfg::detail
