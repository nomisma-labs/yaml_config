# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
"""Read and write the shared YAML config value profile without implicit YAML 1.1 types."""

import math
import re
from typing import Any

import yaml


minimum_integer = -(1 << 63)
maximum_integer = (1 << 64) - 1
_boolean = {"true": True, "True": True, "TRUE": True,
            "false": False, "False": False, "FALSE": False}
_null = {"", "~", "null", "Null", "NULL"}
_nonfinite = re.compile(r"(?:[+-]?\.(?:inf|Inf|INF)|\.(?:nan|NaN|NAN))\Z")
_decimal = re.compile(r"(?:0|[1-9][0-9]*|[+-][0-9]+)\Z")
_float = re.compile(r"[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?\Z")


def _scalar(text: str, style: str | None) -> Any:
    if style is not None:
        return text
    if text in _null:
        raise ValueError("null is unsupported; omit optional keys or quote an empty string")
    if text in _boolean:
        return _boolean[text]
    if _nonfinite.fullmatch(text):
        raise ValueError("expected a finite number")
    base = 0
    digits = text.lstrip("+-")
    if _decimal.fullmatch(text):
        base = 10
        # The pinned parser's integer resolver bounds the digit count before conversion.
        if len(digits) > (19 if text.startswith("-") else 20):
            return text
    elif re.fullmatch(r"0o[0-7]+", text):
        base, digits = 8, text[2:]
        if len(digits) > 22:
            return text
    elif re.fullmatch(r"0x[0-9a-fA-F]+", text):
        base, digits = 16, text[2:]
        if len(digits) > 16:
            return text
    if base:
        value = int(text, base)
        return value if minimum_integer <= value <= maximum_integer else text
    if _float.fullmatch(text) and any(c in text for c in ".eE"):
        # An unsigned leading-zero token is a string in fkYAML, even with a decimal point.
        if len(text) > 1 and text[0] == "0" and text[1].isdigit():
            return text
        value = float(text)
        nonzero_mantissa = any(c in "123456789" for c in re.split("[eE]", text)[0])
        if math.isfinite(value) and (value != 0 or not nonzero_mantissa):
            return value
    return text


def _pointer(parent: str, key: str | int) -> str:
    return parent + "/" + str(key).replace("~", "~0").replace("/", "~1")


def validate_values(value: Any, pointer: str = "") -> None:
    """Validate constructed values too, so substitutions and serialization cannot bypass the profile."""
    if type(value) is str:
        value.encode("utf-8")
    elif type(value) is bool:
        pass
    elif type(value) is int:
        if not minimum_integer <= value <= maximum_integer:
            raise ValueError(f"{pointer}: integer outside the supported 64-bit range")
    elif type(value) is float:
        if not math.isfinite(value):
            raise ValueError(f"{pointer}: expected a finite number")
    elif type(value) is list:
        for index, item in enumerate(value):
            validate_values(item, _pointer(pointer, index))
    elif type(value) is dict:
        for key, item in value.items():
            if type(key) is not str:
                raise ValueError(f"{pointer}: mapping keys must be strings")
            if key == "<<":
                raise ValueError(f"{pointer}: YAML merge keys are unsupported")
            key.encode("utf-8")
            validate_values(item, _pointer(pointer, key))
    else:
        raise ValueError(f"{pointer}: unsupported YAML value {type(value).__name__}; null is unsupported")


def loads(text: str | bytes, label: str = "<config>") -> Any:
    """Read one YAML document, retaining source context in profile and syntax errors."""
    try:
        if isinstance(text, bytes):
            text = text.decode("utf-8")
        text.encode("utf-8")
        events = iter(yaml.parse(text, Loader=yaml.BaseLoader))
        assert isinstance(next(events), yaml.events.StreamStartEvent)
        start = next(events)
        if not isinstance(start, yaml.events.DocumentStartEvent):
            raise ValueError("expected exactly one YAML document")
        if start.version not in (None, (1, 2)):
            raise ValueError("only YAML 1.2 is supported")

        def read(event, pointer: str):
            def fail(message):
                mark = event.start_mark
                raise ValueError(f"{pointer or '<root>'} (line {mark.line + 1}, column {mark.column + 1}): {message}")
            if isinstance(event, yaml.events.AliasEvent) or getattr(event, "anchor", None) is not None:
                fail("anchors and aliases are unsupported")
            if getattr(event, "tag", None) is not None:
                fail("explicit YAML tags are unsupported")
            if isinstance(event, yaml.events.ScalarEvent):
                try:
                    return _scalar(event.value, event.style)
                except ValueError as error:
                    fail(str(error))
            if isinstance(event, yaml.events.SequenceStartEvent):
                result = []
                for child in events:
                    if isinstance(child, yaml.events.SequenceEndEvent):
                        return result
                    result.append(read(child, _pointer(pointer, len(result))))
            if isinstance(event, yaml.events.MappingStartEvent):
                result = {}
                for child in events:
                    if isinstance(child, yaml.events.MappingEndEvent):
                        return result
                    key = read(child, pointer)
                    if type(key) is not str:
                        fail("mapping keys must be strings")
                    if key == "<<":
                        fail("YAML merge keys are unsupported")
                    if key in result:
                        fail(f"duplicate mapping key {key!r}")
                    result[key] = read(next(events), _pointer(pointer, key))
            fail("expected a YAML value")

        value = read(next(events), "")
        if not isinstance(next(events), yaml.events.DocumentEndEvent):
            raise ValueError("expected end of YAML document")
        if not isinstance(next(events), yaml.events.StreamEndEvent):
            raise ValueError("expected exactly one YAML document")
        validate_values(value)
        return value
    except (yaml.YAMLError, ValueError, UnicodeError, RecursionError, StopIteration) as error:
        raise ValueError(f"{label}: {error}") from error


class _dumper(yaml.SafeDumper):
    def ignore_aliases(self, data):
        return True

    def increase_indent(self, flow=False, indentless=False):
        return super().increase_indent(flow, False)


def _represent_string(dumper, value):
    # Always quote strings: their type must survive both parsers, including numeric-looking text.
    return dumper.represent_scalar("tag:yaml.org,2002:str", value, style='"')


_dumper.add_representer(str, _represent_string)


def dumps(value: Any) -> str:
    """Emit deterministic, value-preserving YAML; shared object identity is not serialized."""
    validate_values(value)
    return yaml.dump(value, Dumper=_dumper, allow_unicode=True, sort_keys=True,
                     default_flow_style=False, width=100)
