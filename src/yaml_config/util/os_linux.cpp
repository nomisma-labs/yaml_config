// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include "os.h"

import std;

namespace nom::ycfg::os
{
    std::filesystem::path executable_path()
    {
        return std::filesystem::read_symlink("/proc/self/exe");
    }
} // namespace nom::ycfg::os
