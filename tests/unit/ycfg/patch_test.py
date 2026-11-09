#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
"""The compatibility overlay must fail closed on dependency or patch drift."""

from pathlib import Path
import shutil
import tempfile
import unittest

from tools.yaml_config.prepare_fkyaml import apply_patch, prepare

repository = Path(__file__).resolve().parents[3]
patch = repository / "cmake/yaml_config/patches/fkyaml_compatibility.patch"


class patch_test(unittest.TestCase):
    def test_pinned_overlay_is_repeatable_and_leaves_source_unchanged(self):
        source = repository / "external/fkyaml"
        original = (source / "include/fkYAML/detail/conversions/scalar_conv.hpp").read_bytes()
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            prepare(source, output, patch)
            header = output / "fkYAML/detail/conversions/scalar_conv.hpp"
            self.assertIn(b"FK_YAML_NOMISMA_COMPATIBILITY_PATCH", header.read_bytes())
            timestamp = header.stat().st_mtime_ns
            prepare(source, output, patch)
            self.assertEqual(header.stat().st_mtime_ns, timestamp)
        self.assertEqual((source / "include/fkYAML/detail/conversions/scalar_conv.hpp").read_bytes(), original)

    def test_dependency_drift_is_rejected_before_copy(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "source"
            shutil.copytree(repository / "external/fkyaml/include", source / "include")
            header = source / "include/fkYAML/detail/conversions/scalar_conv.hpp"
            header.write_bytes(header.read_bytes() + b"\n")
            with self.assertRaisesRegex(ValueError, "exact v0.5.0"):
                prepare(source, Path(directory) / "output", patch)
            self.assertFalse((Path(directory) / "output").exists())

    def test_zero_context_insertion_uses_the_previous_line_position(self):
        self.assertEqual(apply_patch("first\nsecond\n", "@@ -1,0 +2 @@\n+insert\n"),
                         "first\ninsert\nsecond\n")

    def test_bad_context_and_missing_patch_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "context mismatch"):
            apply_patch("original\n", "@@ -1 +1 @@\n-wrong\n+new\n")
        with tempfile.TemporaryDirectory() as directory:
            empty = Path(directory) / "empty.patch"
            empty.write_text("")
            with self.assertRaisesRegex(ValueError, "incomplete"):
                prepare(repository / "external/fkyaml", Path(directory) / "output", empty)
