# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_lib(yaml_config IMPORT_STD
    MODULES yaml_config.cppm config_values.cppm config_mapping.cppm
    HEADERS fkyaml.h config.h foreach.h
    PUBLIC_DEPS nom::yaml_config_fkyaml nom::config_types)
nom_install_package(TARGETS yaml_config yaml_config_fkyaml DEPENDENCIES base)
