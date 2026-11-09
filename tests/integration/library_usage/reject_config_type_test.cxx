// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <yaml_config/config.h>

import std;

import nom.yaml_config;

namespace rejected_fixture
{
    struct unregistered_type
    {
        int value{};
    };

    inline void from_node(const fkyaml::node& document, unregistered_type& destination)
    {
        destination.value = document.get_value<int>();
    }

    inline void to_node(fkyaml::node& document, const unregistered_type& source)
    {
        document = source.value;
    }

    struct config
    {
#if defined(NOM_TEST_INTEGER_MEMBER)
        int value{};
#elif defined(NOM_TEST_INTEGER_CONTAINER)
        std::vector<int> value;
#elif defined(NOM_TEST_OPTIONAL_CONTAINER)
        std::vector<std::optional<nom::cfg_int>> value;
#elif defined(NOM_TEST_DEFAULTED_OPTIONAL)
        nom::cfg_defaulted<std::optional<nom::cfg_int>> value;
#elif defined(NOM_TEST_UNREGISTERED_TYPE)
        unregistered_type value;
#else
#error Select a disabled config type test
#endif
    };

    NOM_YCONFIG(config, value)
} // namespace rejected_fixture

int main()
{
    const fkyaml::node document(fkyaml::node::mapping_type{{"value", fkyaml::node(1)}});
    const auto config = document.get_value<rejected_fixture::config>();
    const fkyaml::node serialized = config;
    return serialized.empty();
}
