# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

cmake_minimum_required(VERSION 4.4)
get_filename_component(yaml_config_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include(${yaml_config_root}/cmake/kit/std_modules.cmake)
project(yaml_parser_research LANGUAGES CXX)
include(${yaml_config_root}/cmake/kit/nom_kit.cmake)
nom_project_options()
if(NOT EXISTS "${FK_YAML_SOURCE_DIR}/include/fkYAML/node.hpp")
    message(FATAL_ERROR "Set FK_YAML_SOURCE_DIR to an fkYAML v0.5.0 checkout")
endif()
nom_lib(parser_probe IMPORT_STD MODULES src/parser_probe/parser_probe.cppm)
target_include_directories(parser_probe SYSTEM PUBLIC "${FK_YAML_SOURCE_DIR}/include")
nom_app(parser_probe_runner IMPORT_STD SOURCES main.cpp DEPS nom::parser_probe)
