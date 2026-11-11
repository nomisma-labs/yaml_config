#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: MIT
"""Compare Python values and rejection behavior with the compiled C++ codec."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


repository = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repository))
from tools.yaml_config import yaml_codec


def projection(value):
    if isinstance(value, dict):
        return ("map", sorted((key, projection(item)) for key, item in value.items()))
    if isinstance(value, list):
        return ("seq", [projection(item) for item in value])
    if type(value) is float:
        return ("float", value.hex())
    return (type(value).__name__, value)


def main():
    cases = json.loads((repository / "tests/unit/ycfg/value_cases.json").read_text())
    fixtures = [*repository.glob("docs/yaml_config/examples/**/*.yaml"),
                *repository.glob("apps/*/config/*.yaml"),
                repository / "tests/integration/library_usage/consumer_ycfg.yaml"]
    cases.extend({"name": str(path.relative_to(repository)), "yaml": path.read_text(),
                  "value": yaml_codec.loads(path.read_text())} for path in fixtures)
    with tempfile.TemporaryDirectory() as directory:
        source, output = Path(directory) / "input.yaml", Path(directory) / "output.yaml"
        environment = {**os.environ, "YCFG_VALUE_INPUT": str(source), "YCFG_VALUE_OUTPUT": str(output)}
        for case in cases:
            source.write_text(case["yaml"], encoding="utf-8")
            result = subprocess.run([sys.argv[1], "C++ value codec agrees with Python corpus"],
                                    env=environment, capture_output=True, text=True, timeout=15)
            if "error" in case:
                # A sanitizer crash must never count as a successful rejection.
                if (result.returncode <= 0 or result.stderr
                        or "due to unexpected exception" not in result.stdout):
                    raise AssertionError((case["name"], result.returncode, result.stdout, result.stderr))
                continue
            if result.returncode != 0:
                raise AssertionError((case["name"], result.stdout, result.stderr))
            actual = yaml_codec.loads(output.read_text())
            if projection(actual) != projection(case["value"]):
                raise AssertionError((case["name"], actual, case["value"]))
            # Also check the Python writer's output, including strings that resemble scalars.
            source.write_text(yaml_codec.dumps(case["value"]), encoding="utf-8")
            result = subprocess.run([sys.argv[1], "C++ value codec agrees with Python corpus"],
                                    env=environment, capture_output=True, text=True, timeout=15)
            if result.returncode != 0 or projection(yaml_codec.loads(output.read_text())) != projection(case["value"]):
                raise AssertionError((case["name"], "Python emission", result.stdout, result.stderr))
    print(f"PASS: {len(cases)} shared corpus and documentation cases agree across Python and C++")


if __name__ == "__main__":
    main()
