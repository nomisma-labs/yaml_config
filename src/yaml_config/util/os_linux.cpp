// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#include "os.h"

import std;

namespace nom::ycfg::os
{
    std::filesystem::path executable_path()
    {
        return std::filesystem::read_symlink("/proc/self/exe");
    }
} // namespace nom::ycfg::os
