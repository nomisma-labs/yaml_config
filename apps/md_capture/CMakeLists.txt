# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_app(md_capture IMPORT_STD SOURCES main.cpp DEPS nom::yaml_config
    COMPILE_OPTIONS -DNOM_CONFIG_KEY=md_capture)

nom_ycfg_generate(md_capture SOURCE md_capture_ycfg.yaml BUILD_WITH_APP)
nom_ycfg_test(md_capture base)
nom_ycfg_test(md_capture ci)
nom_ycfg_test(md_capture prod)
nom_ycfg_test(md_capture prod-liquidation)
