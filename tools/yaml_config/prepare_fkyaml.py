#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
"""Prepare a private fkYAML include tree with the reviewed, hash-guarded numeric patch."""

import argparse
import hashlib
from pathlib import Path
import re


# fkYAML v0.5.0, commit 5cc0747068b94708aa0986cacd200aa74be4f9e8.
original_sha256 = {
    "fkYAML/detail/conversions/scalar_conv.hpp": "6500fd9d49f21a976c561079b860ae295c87ca3345a076d46a5684223d1a9ecf",
    "fkYAML/detail/conversions/to_string.hpp": "2d06a4616aab51224606fe270d3f9ba967e060b7a1cbd2567575dafcc0fc7b2d",
}

def apply_patch(original: str, patch: str) -> str:
    source = original.splitlines(keepends=True)
    result = []
    position = 0
    lines = iter(patch.splitlines(keepends=True))
    for line in lines:
        if line.startswith(("--- ", "+++ ")):
            continue
        if line.startswith("@@"):
            match = re.match(r"@@ -(\d+)(?:,(\d+))? \+\d+(?:,\d+)? @@", line)
            if not match:
                raise ValueError("invalid patch hunk")
            start = int(match[1]) - (0 if match[2] == "0" else 1)
            if start < position:
                raise ValueError("overlapping patch hunks")
            result.extend(source[position:start])
            position = start
        elif line.startswith((" ", "-")):
            if position >= len(source) or source[position] != line[1:]:
                raise ValueError("fkYAML patch context mismatch")
            if line[0] == " ":
                result.append(source[position])
            position += 1
        elif line.startswith("+"):
            result.append(line[1:])
        else:
            raise ValueError("unexpected patch line")
    result.extend(source[position:])
    return "".join(result)


def prepare(source: Path, output: Path, patch: Path) -> None:
    patches = re.split(r"(?=^--- a/include/)", patch.read_text(encoding="utf-8"), flags=re.MULTILINE)[1:]
    modified = {}
    for section in patches:
        name = section.splitlines()[0].removeprefix("--- a/include/")
        if name not in original_sha256 or name in modified:
            raise ValueError("unexpected or duplicate fkYAML patch target")
        original = (source / "include" / name).read_bytes()
        if hashlib.sha256(original).hexdigest() != original_sha256[name]:
            raise ValueError("fkYAML numeric patch requires exact v0.5.0 headers; review dependency changes")
        modified[Path(name)] = apply_patch(original.decode("utf-8"), section).encode("utf-8")
    if set(map(str, modified)) != set(original_sha256):
        raise ValueError("incomplete fkYAML numeric patch")
    scalar = Path("fkYAML/detail/conversions/scalar_conv.hpp")
    marker = b"#define FK_YAML_CONVERSIONS_SCALAR_CONV_HPP\n"
    if modified[scalar].count(marker) != 1:
        raise ValueError("fkYAML patch marker location mismatch")
    modified[scalar] = modified[scalar].replace(marker, marker + b"#define FK_YAML_NOMISMA_NUMERIC_PATCH 1\n")
    for header in (source / "include").rglob("*"):
        if not header.is_file():
            continue
        relative = header.relative_to(source / "include")
        content = modified.get(relative, header.read_bytes())
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.is_file() or target.read_bytes() != content:
            target.write_bytes(content)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--patch", required=True, type=Path)
    args = parser.parse_args()
    prepare(args.source, args.output, args.patch)


if __name__ == "__main__":
    main()
