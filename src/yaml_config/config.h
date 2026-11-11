// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>
#include <yaml_config/fkyaml.h>
#include <yaml_config/foreach.h>

// Evaluate the application's definition at its call site, outside compiled library code.
#define NOM_YCFG_STRINGIFY_IMPL(value) #value
#define NOM_YCFG_STRINGIFY(value) NOM_YCFG_STRINGIFY_IMPL(value)
#ifdef NOM_CONFIG_KEY
#define NOM_CONFIG_KEY_VALUE NOM_YCFG_STRINGIFY(NOM_CONFIG_KEY)
#else
#define NOM_CONFIG_KEY_VALUE ""
#endif

#define NOM_YCFG_READ_MEMBER(member)                                                                                   \
    static_assert(::nom::is_config_member<std::remove_cvref_t<decltype(nom_ycfg_destination.member)>>,                 \
                  "NOM_YCONFIG member type is not config-enabled");                                                    \
    ::nom::ycfg::detail::read_member(nom_ycfg_document, #member, nom_ycfg_destination.member);
#define NOM_YCFG_WRITE_MEMBER(member)                                                                                  \
    static_assert(::nom::is_config_member<std::remove_cvref_t<decltype(nom_ycfg_source.member)>>,                      \
                  "NOM_YCONFIG member type is not config-enabled");                                                    \
    ::nom::ycfg::detail::write_member(nom_ycfg_document, #member, nom_ycfg_source.member);

// Declare beside the type, in its namespace, after importing nom.yaml_config.
#define NOM_YCONFIG(Type, ...)                                                                                         \
    inline std::true_type nom_config_type(Type*)                                                                       \
    {                                                                                                                  \
        return {};                                                                                                     \
    }                                                                                                                  \
    inline void from_node(const ::fkyaml::node& nom_ycfg_document, [[maybe_unused]] Type& nom_ycfg_destination)        \
    {                                                                                                                  \
        (void)nom_ycfg_document.as_map();                                                                              \
        __VA_OPT__(NOM_YCFG_FOR_EACH(NOM_YCFG_READ_MEMBER, __VA_ARGS__))                                               \
    }                                                                                                                  \
    inline void to_node(::fkyaml::node& nom_ycfg_document, [[maybe_unused]] const Type& nom_ycfg_source)               \
    {                                                                                                                  \
        nom_ycfg_document = ::fkyaml::node::mapping();                                                                 \
        __VA_OPT__(NOM_YCFG_FOR_EACH(NOM_YCFG_WRITE_MEMBER, __VA_ARGS__))                                              \
    }

// Declare in the enum's namespace with {{Type::value, "name"}, ...}.
// Matching is exact; the first name for a value is its serialized form.
#define NOM_YCONFIG_ENUM(Type, ...)                                                                                    \
    static_assert(std::is_enum_v<Type>, "NOM_YCONFIG_ENUM requires an enum type");                                     \
    inline std::true_type nom_config_type(Type*)                                                                       \
    {                                                                                                                  \
        return {};                                                                                                     \
    }                                                                                                                  \
    inline void from_node(const ::fkyaml::node& nom_ycfg_document, Type& nom_ycfg_destination)                         \
    {                                                                                                                  \
        static constexpr std::pair<Type, std::string_view> nom_ycfg_names[] = __VA_ARGS__;                             \
        const auto& nom_ycfg_name = nom_ycfg_document.as_str();                                                        \
        for (const auto& [nom_ycfg_value, nom_ycfg_text] : nom_ycfg_names)                                             \
        {                                                                                                              \
            if (nom_ycfg_name == nom_ycfg_text)                                                                        \
            {                                                                                                          \
                nom_ycfg_destination = nom_ycfg_value;                                                                 \
                return;                                                                                                \
            }                                                                                                          \
        }                                                                                                              \
        throw ::nom::ycfg::mapping_error("unmapped " #Type " value " + nom_ycfg_document.as_str());                    \
    }                                                                                                                  \
    inline void to_node(::fkyaml::node& nom_ycfg_document, Type nom_ycfg_source)                                       \
    {                                                                                                                  \
        static constexpr std::pair<Type, std::string_view> nom_ycfg_names[] = __VA_ARGS__;                             \
        for (const auto& [nom_ycfg_value, nom_ycfg_text] : nom_ycfg_names)                                             \
        {                                                                                                              \
            if (nom_ycfg_source == nom_ycfg_value)                                                                     \
            {                                                                                                          \
                nom_ycfg_document = std::string(nom_ycfg_text);                                                        \
                return;                                                                                                \
            }                                                                                                          \
        }                                                                                                              \
        throw ::nom::ycfg::mapping_error("unmapped " #Type " value " +                                                 \
                                         std::to_string(std::to_underlying(nom_ycfg_source)));                         \
    }
