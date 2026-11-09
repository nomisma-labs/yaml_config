# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
"""Shared value-profile cases; the C++ parity test consumes the same corpus."""

import json
import math
from pathlib import Path
import unittest

from tools.yaml_config import yaml_codec


cases_path = Path(__file__).with_name("value_cases.json")


def projection(value):
    if isinstance(value, dict):
        return ("map", sorted((key, projection(item)) for key, item in value.items()))
    if isinstance(value, list):
        return ("seq", [projection(item) for item in value])
    if type(value) is float:
        return ("float", value.hex())
    return (type(value).__name__, value)


class yaml_values_tests(unittest.TestCase):
    def test_shared_value_corpus(self):
        for case in json.loads(cases_path.read_text()):
            with self.subTest(case=case["name"]):
                if "error" in case:
                    with self.assertRaises(ValueError):
                        yaml_codec.loads(case["yaml"], case["name"])
                else:
                    value = yaml_codec.loads(case["yaml"], case["name"])
                    self.assertEqual(projection(value), projection(case["value"]))
                    self.assertEqual(projection(yaml_codec.loads(yaml_codec.dumps(value))), projection(value))

    def test_sorted_emission_and_shared_subtrees_do_not_create_aliases(self):
        shared = {"z": True, "a": "true"}
        text = yaml_codec.dumps({"b": shared, "a": shared})
        self.assertNotIn("&", text)
        self.assertNotIn("*", text)
        self.assertLess(text.index('"a"'), text.index('"b"'))
        self.assertEqual(yaml_codec.loads(text), {"a": shared, "b": shared})

    def test_constructed_values_cannot_bypass_profile(self):
        cycle = []
        cycle.append(cycle)
        for value in (None, math.nan, math.inf, -(1 << 63) - 1, 1 << 64,
                      {1: "key"}, {"<<": {}}, "\ud800", object(), cycle):
            with self.subTest(value=repr(value)):
                with self.assertRaises((ValueError, UnicodeError, RecursionError)):
                    yaml_codec.dumps(value)

    def test_errors_include_source_pointer_and_location(self):
        with self.assertRaisesRegex(ValueError, r"settings.yaml.*?/outer/0.*?line 2"):
            yaml_codec.loads("outer:\n  - null\n", "settings.yaml")

    def test_invalid_utf8_and_surrogates_fail(self):
        for value in (b"key: \xff", "\ud800", '"\\uD800"'):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    yaml_codec.loads(value)


if __name__ == "__main__":
    unittest.main()
