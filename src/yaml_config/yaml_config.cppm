// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

module;

#include "fkyaml.h"

export module nom.yaml_config;

import std;

export import nom.config_types;
export import :values;
export import :mapping;
export import :runtime;

export namespace nom::ycfg
{
    namespace detail
    {
        template <typename Config>
        Config map_loaded_config(const fkyaml::node& document, const config_location& location)
        {
            try
            {
                return document.get_value<Config>();
            }
            catch (const std::bad_alloc&)
            {
                throw;
            }
            catch (const mapping_error& error)
            {
                throw config_error(config_stage::mapping, error.what(), location.path, error.pointer);
            }
            catch (const std::exception& error)
            {
                throw config_error(config_stage::mapping, error.what(), location.path);
            }
        }

    } // namespace detail

    template <typename Config> struct loaded_config
    {
        Config value;
        config_location location;
    };

    template <typename Config>
    void print_config_check(std::ostream& output,
                            const loaded_config<Config>& loaded,
                            config_check_mode mode = config_check_mode::full)
    {
        if (mode == config_check_mode::silent)
        {
            output << "OK\n";
            return;
        }

        std::string document;
        try
        {
            document = format_config(fkyaml::node(loaded.value));
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const mapping_error& error)
        {
            throw config_error(config_stage::mapping, error.what(), loaded.location.path, error.pointer);
        }
        catch (const std::exception& error)
        {
            throw config_error(config_stage::mapping, error.what(), loaded.location.path);
        }
        if (mode == config_check_mode::full)
        {
            if (!loaded.location.search_details.empty())
            {
                output << "Config lookup:\n" << loaded.location.search_details;
            }
            output << "Config OK (" << config_source_name(loaded.location.source) << ')';
            if (loaded.location.path)
            {
                output << ": " << loaded.location.path->string();
            }
            output << "\nConfiguration:\n";
        }
        output << document << '\n';
    }

    template <typename Config>
    loaded_config<Config> load_config(const config_options& options = {},
                                      const environment_lookup& environment = process_environment)
    {
        static_assert(
            requires(const Config& value) {
                { value.validate() } -> std::same_as<std::expected<void, std::string>>;
            }, "The root config must provide validate() const returning std::expected<void, std::string>");

        auto location = locate_config(options, environment);
        auto document = read_config(location);
        substitute_runtime_variables(document, environment, location);
        if (location.source == config_source::hard_wired && !options.overrides.empty())
        {
            // C++ defaults provide the existing YAML targets for hard-wired overrides.
            auto initial = detail::map_loaded_config<Config>(document, location);
            try
            {
                document = fkyaml::node(initial);
            }
            catch (const std::bad_alloc&)
            {
                throw;
            }
            catch (const mapping_error& error)
            {
                throw config_error(config_stage::mapping, error.what(), location.path, error.pointer);
            }
            catch (const std::exception& error)
            {
                throw config_error(config_stage::mapping, error.what(), location.path);
            }
        }
        apply_config_overrides(
            document,
            options.overrides,
            [](const fkyaml::node& candidate) { (void)candidate.get_value<Config>(); },
            location);
        auto value = detail::map_loaded_config<Config>(document, location);
        std::expected<void, std::string> validation;
        try
        {
            validation = std::as_const(value).validate();
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const std::exception& error)
        {
            throw config_error(config_stage::validation, error.what(), location.path);
        }
        if (!validation)
        {
            throw config_error(config_stage::validation, validation.error(), location.path);
        }
        return {std::move(value), std::move(location)};
    }
} // namespace nom::ycfg
