# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_lib(yaml_config IMPORT_STD
    MODULES yaml_config.cppm config_values.cppm
    HEADERS fkyaml.h
    PUBLIC_DEPS nom::yaml_config_fkyaml)
nom_install_package(TARGETS yaml_config yaml_config_fkyaml)
