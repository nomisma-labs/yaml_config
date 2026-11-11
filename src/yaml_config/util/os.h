// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>

namespace nom::ycfg::os
{
    // Private platform boundary; a future common OS utility can own this operation.
    std::filesystem::path executable_path();
} // namespace nom::ycfg::os
