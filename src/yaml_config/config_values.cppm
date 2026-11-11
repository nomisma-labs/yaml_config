// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

module;
#include "fkyaml.h"
export module nom.yaml_config:values;
import std;

export namespace nom::ycfg
{
    class mapping_error : public std::runtime_error
    {
    public:
        explicit mapping_error(std::string message, std::string pointer = {})
            : std::runtime_error(std::move(message)), pointer(std::move(pointer))
        {
        }
        std::string pointer;
    };

    namespace detail
    {
        inline std::string pointer_token(std::string_view token)
        {
            std::string result = "/";
            for (const char c : token)
            {
                if (c == '~')
                    result += "~0";
                else if (c == '/')
                    result += "~1";
                else
                    result += c;
            }
            return result;
        }
        template <typename Operation> decltype(auto) at_member(std::string_view token, Operation&& operation)
        {
            try
            {
                return operation();
            }
            catch (const std::bad_alloc&)
            {
                throw;
            }
            catch (const mapping_error& error)
            {
                throw mapping_error(error.what(), pointer_token(token) + error.pointer);
            }
            catch (const std::exception& error)
            {
                throw mapping_error(error.what(), pointer_token(token));
            }
        }
        void validate_utf8(std::string_view text)
        {
            for (std::size_t i = 0; i < text.size();)
            {
                const auto first = static_cast<unsigned char>(text[i++]);
                if (first < 0x80)
                    continue;
                unsigned length;
                unsigned code;
                unsigned minimum;
                if (first >= 0xc2 && first <= 0xdf)
                {
                    length = 1;
                    code = first & 0x1f;
                    minimum = 0x80;
                }
                else if (first >= 0xe0 && first <= 0xef)
                {
                    length = 2;
                    code = first & 0x0f;
                    minimum = 0x800;
                }
                else if (first >= 0xf0 && first <= 0xf4)
                {
                    length = 3;
                    code = first & 7;
                    minimum = 0x10000;
                }
                else
                    throw std::runtime_error("invalid UTF-8");
                for (unsigned n = 0; n < length; ++n)
                {
                    if (i == text.size())
                        throw std::runtime_error("truncated UTF-8");
                    const auto c = static_cast<unsigned char>(text[i++]);
                    if ((c & 0xc0) != 0x80)
                        throw std::runtime_error("invalid UTF-8 continuation");
                    code = (code << 6) | (c & 0x3f);
                }
                if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
                    throw std::runtime_error("invalid Unicode scalar");
            }
        }
    } // namespace detail

    void validate_config_values(const fkyaml::node& document)
    {
        if (document.is_alias() || document.has_anchor_name())
            throw mapping_error("anchors and aliases are unsupported");
        if (document.has_tag_name())
            throw mapping_error("explicit YAML tags are unsupported");
        if (document.is_null())
            throw mapping_error("null is unsupported; omit optional keys");
        if (document.is_string())
            detail::validate_utf8(document.as_str());
        else if (document.is_mapping())
            for (const auto& [key, value] : document.as_map())
            {
                if (!key.is_string())
                    throw mapping_error("mapping keys must be strings");
                validate_config_values(key);
                if (key.as_str() == "<<")
                    throw mapping_error("YAML merge keys are unsupported");
                detail::at_member(key.as_str(), [&] { validate_config_values(value); });
            }
        else if (document.is_sequence())
            for (std::size_t i = 0; i < document.size(); ++i)
                detail::at_member(std::to_string(i), [&] { validate_config_values(document.at(i)); });
        else if (document.is_float_number() && !std::isfinite(document.as_float()))
            throw mapping_error("expected a finite number");
    }

    fkyaml::node parse_config(std::string_view source, std::string_view label = "YAML")
    {
        detail::validate_utf8(source);
        for (std::size_t i = 0; i < source.size(); ++i)
        {
            const auto c = static_cast<unsigned char>(source[i]);
            const auto next = i + 1 < source.size() ? static_cast<unsigned char>(source[i + 1]) : 0;
            if ((c < 0x20 && c != '\t' && c != '\n' && c != '\r') || c == 0x7f ||
                (c == 0xc2 && next >= 0x80 && next <= 0x9f && next != 0x85) ||
                (c == 0xef && next == 0xbf && i + 2 < source.size() &&
                 (static_cast<unsigned char>(source[i + 2]) == 0xbe ||
                  static_cast<unsigned char>(source[i + 2]) == 0xbf)))
                throw mapping_error(std::string(label) + ": non-printable character must use a YAML escape");
        }
        std::string text(source);
        if (text.empty() || text.back() != '\n')
            text += '\n';
        try
        {
            // fkYAML accepts future 1.x directives as 1.2. The shared profile pins 1.2.
            auto prefix = std::string_view(text);
            if (prefix.starts_with("\xef\xbb\xbf"))
                prefix.remove_prefix(3);
            while (!prefix.empty())
            {
                const auto end = prefix.find('\n');
                const auto line = prefix.substr(0, end);
                const auto start = line.find_first_not_of(" \t\r");
                if (start != std::string_view::npos && line[start] != '#')
                {
                    if (!line.starts_with('%'))
                        break;
                    if (line.starts_with("%YAML ") || line.starts_with("%YAML\t"))
                    {
                        const auto version_start = line.find_first_not_of(" \t", 5);
                        const auto version =
                            line.substr(version_start, line.find_first_of(" \t\r#", version_start) - version_start);
                        if (version != "1.2")
                            throw mapping_error("only YAML 1.2 is supported");
                    }
                }
                if (end == std::string_view::npos)
                    break;
                prefix.remove_prefix(end + 1);
            }
            auto documents = fkyaml::node::deserialize_docs(text);
            if (documents.size() != 1)
                throw mapping_error("expected exactly one YAML document");
            auto result = std::move(documents.front());
            if (result.get_yaml_version_type() != fkyaml::yaml_version_type::VERSION_1_2)
                throw mapping_error("only YAML 1.2 is supported");
            validate_config_values(result);
            return result;
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const mapping_error& error)
        {
            throw mapping_error(std::string(label) + ": " + error.what(), error.pointer);
        }
        catch (const std::exception& error)
        {
            throw mapping_error(std::string(label) + ": " + error.what());
        }
    }

    std::string format_config(const fkyaml::node& document)
    {
        validate_config_values(document);
        auto text = fkyaml::node::serialize(document);
        if (text.empty() || text.back() != '\n')
            text += '\n';
        (void)parse_config(text);
        return text;
    }
} // namespace nom::ycfg
