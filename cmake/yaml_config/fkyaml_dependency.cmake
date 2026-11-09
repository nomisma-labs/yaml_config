# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

include_guard(GLOBAL)
include(GNUInstallDirs)

# Bundle one patched header provider with this package, without modifying the upstream checkout.
function(yaml_config_find_fkyaml)
    if(TARGET nom::yaml_config_fkyaml)
        return()
    endif()
    nom_require_submodule(external/fkyaml)
    find_package(Python 3.11 REQUIRED COMPONENTS Interpreter)
    set(source "${PROJECT_SOURCE_DIR}/external/fkyaml")
    set(headers "${PROJECT_BINARY_DIR}/fkyaml/include")
    set(patch "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/patches/fkyaml_compatibility.patch")
    set(script "${PROJECT_SOURCE_DIR}/tools/yaml_config/prepare_fkyaml.py")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${patch}" "${script}" "${source}/include/fkYAML/detail/conversions/scalar_conv.hpp"
        "${source}/include/fkYAML/detail/conversions/to_string.hpp"
        "${source}/include/fkYAML/detail/input/deserializer.hpp"
        "${source}/include/fkYAML/detail/encodings/yaml_escaper.hpp"
        "${source}/include/fkYAML/detail/output/serializer.hpp")
    execute_process(COMMAND "${Python_EXECUTABLE}" "${script}"
        --source "${source}" --output "${headers}" --patch "${patch}"
        RESULT_VARIABLE status ERROR_VARIABLE diagnostic)
    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "Cannot prepare the pinned fkYAML compatibility headers: ${diagnostic}")
    endif()
    nom_lib(yaml_config_fkyaml)
    target_include_directories(yaml_config_fkyaml SYSTEM INTERFACE
        $<BUILD_INTERFACE:${headers}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    if(nom_install)
        install(DIRECTORY "${headers}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        install(FILES "${source}/LICENSE.txt" "${patch}"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/fkyaml)
    endif()
endfunction()
