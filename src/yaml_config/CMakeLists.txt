# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: MIT

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(yaml_config_os_source util/os_linux.cpp)
else()
    message(FATAL_ERROR "yaml_config executable discovery is not implemented for ${CMAKE_SYSTEM_NAME}")
endif()

nom_lib(yaml_config IMPORT_STD
    MODULES yaml_config.cppm config_mapping.cppm config_runtime.cppm config_values.cppm
    SOURCES config_options.cpp config_locator.cpp config_loader.cpp ${yaml_config_os_source}
    HEADERS config.h fkyaml.h foreach.h
    PUBLIC_DEPS nom::config_types nom::yaml_config_fkyaml
    PRIVATE_DEPS Boost::program_options)

nom_install_package(TARGETS yaml_config yaml_config_fkyaml DEPENDENCIES base Boost)
