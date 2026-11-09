# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(values IMPORT_STD SOURCES value_codec_test.cxx DEPS nom::yaml_config)
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter)
add_test(NAME ${PROJECT_NAME}_value_parity
    COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/parity_test.py"
        $<TARGET_FILE:${PROJECT_NAME}_values>)
set_tests_properties(${PROJECT_NAME}_value_parity PROPERTIES LABELS unit)
