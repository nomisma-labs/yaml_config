# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
"""The educational examples and focused language contracts for ycfg resolution."""

import json
from tools.yaml_config import yaml_codec
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.yaml_config.ycfg import config_error, generate_configs


repository = Path(__file__).resolve().parents[3]
examples = repository / "docs/yaml_config/examples"


class example_tests(unittest.TestCase):
    def test_all_documented_expected_outputs(self):
        sources = sorted((source, source.parent / "genconfig")
                         for source in examples.glob("*/*_ycfg.yaml")
                         if (source.parent / "genconfig").is_dir())
        sources.append((repository / "apps/md_capture/config/md_capture_ycfg.yaml",
                        examples / "complete_config/genconfig"))
        self.assertEqual(len(sources), 10)
        for source, expected_dir in sources:
            with self.subTest(example=source.parent.name):
                expected_files = set(expected_dir.rglob("*.yaml"))
                actual_files = set()
                for name, result in generate_configs(source, environment={}).items():
                    self.assertEqual(name, result.name)
                    expected = expected_dir / name / result.gen_file
                    if not expected.is_file():
                        expected = expected_dir / result.gen_file
                    self.assertTrue(expected.is_file(), str(expected))
                    self.assertEqual(result.config, yaml_codec.loads(expected.read_text()))
                    actual_files.add(expected)
                self.assertEqual(actual_files, expected_files)

    def test_structure_example(self):
        source = examples / "structure_config/app_ycfg.yaml"
        results = generate_configs(source, environment={})
        self.assertEqual(list(results), ["ci", "prod"])
        self.assertEqual({name: item.gen_file for name, item in results.items()},
                         {"ci": "app.yaml", "prod": "prod.yaml"})
        expected = yaml_codec.loads(source.with_name("app.yaml").read_text())
        expected["log"]["level"] = "warn"
        self.assertEqual(results["ci"].config, expected)
        expected["subscription"]["symbols"] = yaml_codec.loads(
            source.with_name("symbol.yaml").read_text())
        self.assertEqual(results["prod"].config, expected)


class resolver_tests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.directory = Path(directory.name)
        self.source = self.directory / "app_ycfg.yaml"

    def write_yaml(self, name, content):
        destination = self.directory / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(content), encoding="utf-8")
        return destination

    def resolve_document(self, content, **options):
        self.write_yaml(self.source.name, content)
        options.setdefault("environment", {})
        return generate_configs(self.source, **options)

    def assert_failure(self, content, *details, **options):
        with self.assertRaises(config_error) as caught:
            self.resolve_document(content, **options)
        message = str(caught.exception)
        self.assertIn(self.source.name, message)
        for detail in details:
            self.assertIn(detail, message)
        return message

    def test_single_object_and_single_section_array_are_equivalent(self):
        content = {"ycfg": {"name": "simulation"}, "enabled": True}
        for document in (content, [content]):
            with self.subTest(document=document):
                results = self.resolve_document(document)
                self.assertEqual(list(results), ["simulation"])
                result = results["simulation"]
                self.assertEqual((result.name, result.gen_file, result.config),
                                 ("simulation", "app.yaml", {"enabled": True}))

    def test_ycfg_metadata_name_is_reserved_only_at_section_root(self):
        result = self.resolve_document({
            "ycfg": {"name": "dev"}, "nested": {"ycfg": {"name": "application data"}},
        })["dev"]
        self.assertEqual(result.config, {"nested": {"ycfg": {"name": "application data"}}})

    def test_explicit_output_filename_does_not_follow_import_name(self):
        self.write_yaml("donor.yaml", {"value": 1})
        result = self.resolve_document({
            "ycfg": {"name": "dev", "gen_file": "capture.yaml", "import": "donor.yaml"}
        })["dev"]
        self.assertEqual(result.gen_file, "capture.yaml")

    def test_invalid_document_and_section_shapes(self):
        for document in (None, False, "text", 2, [], [1], {}, {"ycfg": []},
                         {"ycfg": {}}, {"ycfg": {"name": 1}}):
            with self.subTest(document=document):
                self.assert_failure(document)

    def test_invalid_inheritance(self):
        documents = [
            [{"ycfg": {"name": "dev", "extend": "dev"}}],
            [{"ycfg": {"name": "dev"}}, {"ycfg": {"name": "dev", "extend": "dev"}}],
            [{"ycfg": {"name": "dev"}}, {"ycfg": {"name": "ci"}}],
            [{"ycfg": {"name": "dev"}}, {"ycfg": {"name": "ci", "extend": "missing"}}],
            [{"ycfg": {"name": "dev"}}, {"ycfg": {"name": "ci", "extend": ["dev"]}}],
            [{"ycfg": {"name": "dev"}}, {"ycfg": {"name": "ci", "extend": "prod"}},
             {"ycfg": {"name": "prod", "extend": "dev"}}],
        ]
        for document in documents:
            with self.subTest(document=document):
                self.assert_failure(document)

    def test_invalid_metadata_types(self):
        for field, value in (("generate", 1), ("generate", "false"),
                             ("gen_file", None), ("import", {}), ("import", [1])):
            with self.subTest(field=field, value=value):
                self.assert_failure({"ycfg": {"name": "dev", field: value}}, field)

    def test_identity_parent_and_output_metadata_are_literal(self):
        for field in ("name", "gen_file", "extend"):
            with self.subTest(field=field):
                content = [{"ycfg": {"name": "dev"}},
                           {"ycfg": {"name": "ci", "extend": "dev"}}]
                content[-1]["ycfg"][field] = "{ycfg:gen-var VALUE}"
                self.assert_failure(content, field, environment={"VALUE": "dev"})

    def test_output_metadata_rejects_absolute_paths_and_traversal(self):
        for field in ("name", "gen_file"):
            for value in ("../outside.yaml", "/tmp/outside.yaml", "part/../../outside.yaml"):
                with self.subTest(field=field, value=value):
                    self.assert_failure({"ycfg": {"name": "dev", field: value}}, field)

    def test_selection_keeps_source_order_and_resolves_ancestors(self):
        document = [
            {"ycfg": {"name": "common", "generate": False, "gen_file": "shared.yaml"},
             "first": 1},
            {"ycfg": {"name": "ci", "extend": "common"}, "second": 2},
            {"ycfg": {"name": "prod", "extend": "ci"}, "third": 3},
        ]
        results = self.resolve_document(document, sections=["prod", "ci"])
        self.assertEqual(list(results), ["ci", "prod"])
        self.assertEqual({name: item.gen_file for name, item in results.items()},
                         {"ci": "shared.yaml", "prod": "shared.yaml"})
        self.assertEqual(results["prod"].config, {"first": 1, "second": 2, "third": 3})
        selected = self.resolve_document(document, sections=["prod"])
        self.assertEqual(list(selected), ["prod"])
        self.assertEqual(selected["prod"], results["prod"])

    def test_explicit_unknown_or_suppressed_selection_fails(self):
        document = {"ycfg": {"name": "common", "generate": False}}
        for selected in ("missing", "common"):
            with self.subTest(selected=selected):
                self.assert_failure(document, selected, sections=[selected])

    def test_no_generating_sections_needs_no_external_values(self):
        self.assertEqual(self.resolve_document({
            "ycfg": {"name": "common", "generate": False, "import": "absent.yaml"},
            "secret": "{ycfg:gen-var MISSING}",
        }), {})

    def test_unselected_section_semantics_are_not_evaluated(self):
        document = [
            {"ycfg": {"name": "dev"}, "value": 1},
            {"ycfg": {"name": "prod", "extend": "dev", "import": "absent.yaml"},
             "value": "ycfg:unknown"},
        ]
        results = self.resolve_document(document, sections=["dev"])
        self.assertEqual(list(results), ["dev"])
        self.assertEqual(results["dev"].config, {"value": 1})

    def test_unselected_metadata_is_still_validated(self):
        self.assert_failure([
            {"ycfg": {"name": "dev"}},
            {"ycfg": {"name": "prod", "extend": "missing", "generate": False}},
        ], "prod", sections=["dev"])

    def test_selected_child_evaluates_suppressed_parent(self):
        self.assert_failure([
            {"ycfg": {"name": "common", "generate": False},
             "secret": "{ycfg:gen-var MISSING}"},
            {"ycfg": {"name": "prod", "extend": "common"}},
        ], "common", "secret", "MISSING", sections=["prod"])

    def test_recursive_merge_replaces_other_types_and_preserves_siblings(self):
        results = self.resolve_document([
            {"ycfg": {"name": "dev"}, "object": {"keep": 1, "change": 2},
             "array": [1, 2], "scalar": 3},
            {"ycfg": {"name": "ci", "extend": "dev"},
             "object": {"change": 5}, "array": [6], "scalar": {"new": 7}},
            {"ycfg": {"name": "prod", "extend": "dev"}, "object": 8, "array": False},
        ])
        self.assertEqual(results["dev"].config,
                         {"object": {"keep": 1, "change": 2}, "array": [1, 2], "scalar": 3})
        self.assertEqual(results["ci"].config,
                         {"object": {"keep": 1, "change": 5}, "array": [6],
                          "scalar": {"new": 7}})
        self.assertEqual(results["prod"].config, {"object": 8, "array": False, "scalar": 3})
        results["ci"].config["object"]["keep"] = 99
        self.assertEqual(results["dev"].config["object"]["keep"], 1)

    def test_imports_merge_in_order_then_local_values(self):
        self.write_yaml("one.yaml", {"data": {"value": 2, "one": True}})
        self.write_yaml("two.yaml", {"data": {"value": 3, "two": True}})
        results = self.resolve_document([
            {"ycfg": {"name": "dev"}, "data": {"value": 1, "parent": True}},
            {"ycfg": {"name": "ci", "extend": "dev", "import": ["one.yaml", "two.yaml"]},
             "data": {"value": 4}},
        ])
        self.assertEqual(results["ci"].config["data"],
                         {"value": 4, "parent": True, "one": True, "two": True})

    def test_section_import_requires_object(self):
        for donor in ([1], "text", 2, None):
            with self.subTest(donor=donor):
                self.write_yaml("donor.yaml", donor)
                self.assert_failure({"ycfg": {"name": "dev", "import": "donor.yaml"}},
                                    "dev", "donor.yaml")

    def test_property_import_merges_object_and_retains_other_json_types(self):
        self.write_yaml("donor.yaml", {"object": {"new": 2}, "array": [3],
                                       "string": "text", "number": 4,
                                       "boolean": False})
        results = self.resolve_document([
            {"ycfg": {"name": "dev"}, "object": {"keep": 1}},
            {"ycfg": {"name": "ci", "extend": "dev"},
             **{key: f"ycfg:import donor.yaml:/{key}"
                for key in ("object", "array", "string", "number", "boolean")}},
        ])
        self.assertEqual(results["ci"].config,
                         {"object": {"keep": 1, "new": 2}, "array": [3], "string": "text",
                          "number": 4,  "boolean": False})

    def test_imported_json_instructions_and_placeholders_remain_literal(self):
        literal = {"remove": "ycfg:delete", "replace": {"ycfg:override": {"new": 2}},
                   "generation": "{ycfg:gen-var MISSING}",
                   "runtime": "{ycfg:run-var MISSING}", "import": "ycfg:import absent.yaml"}
        self.write_yaml("donor.yaml", literal)
        result = self.resolve_document({
            "ycfg": {"name": "dev", "import": "donor.yaml"},
            "nested": "ycfg:import donor.yaml",
        })["dev"]
        self.assertEqual(result.config, {**literal, "nested": literal})

    def test_imported_override_wrapper_is_literal_during_recursive_merge(self):
        self.write_yaml("donor.yaml", {"node": {"ycfg:override": {"new": 2}}})
        results = self.resolve_document([
            {"ycfg": {"name": "dev"}, "node": {"keep": 1}},
            {"ycfg": {"name": "ci", "extend": "dev", "import": "donor.yaml"}},
        ])
        self.assertEqual(results["ci"].config["node"],
                         {"keep": 1, "ycfg:override": {"new": 2}})

    def test_import_pointer_escapes_empty_key_arrays_and_colons(self):
        self.write_yaml("archive.yaml/markets.yaml", {
            "a/b": {"~key": [{"": {"venue:spot": 42}}]}, "~1": "literal slash escape",
        })
        result = self.resolve_document({
            "ycfg": {"name": "dev"},
            "value": "ycfg:import archive.yaml/markets.yaml:/a~1b/~0key/0//venue:spot",
            "escape_order": "ycfg:import archive.yaml/markets.yaml:/~01",
        })["dev"]
        self.assertEqual(result.config, {"value": 42, "escape_order": "literal slash escape"})

    def test_invalid_pointers_fail(self):
        self.write_yaml("donor.yaml", {"items": ["one"], "scalar": 2})
        for pointer in ("missing_slash", "/absent", "/items/1", "/items/01", "/items/-",
                        "/items/-1", "/items/a", "/scalar/child", "/bad~2escape"):
            with self.subTest(pointer=pointer):
                self.assert_failure({"ycfg": {"name": "dev"},
                                     "value": f"ycfg:import donor.yaml:{pointer}"},
                                    "dev", "value", "donor.yaml")

    def test_relative_and_absolute_import_paths_use_source_location(self):
        donor = self.write_yaml("library/donor.yaml", [1, 2])
        result = self.resolve_document({
            "ycfg": {"name": "dev"}, "relative": "ycfg:import library/donor.yaml",
            "absolute": f"ycfg:import {donor}",
        })["dev"]
        self.assertEqual(result.config, {"relative": [1, 2], "absolute": [1, 2]})

    def test_import_path_can_contain_spaces(self):
        self.write_yaml("config library/market data.yaml", {"name": "BTC"})
        result = self.resolve_document({
            "ycfg": {"name": "dev"},
            "symbol": "ycfg:import config library/market data.yaml:/name",
        })["dev"]
        self.assertEqual(result.config, {"symbol": "BTC"})

    def test_delete_removes_property_and_null_is_rejected(self):
        results = self.resolve_document([
            {"ycfg": {"name": "dev"}, "node": {"keep": 1, "remove": 2}},
            {"ycfg": {"name": "ci", "extend": "dev"}, "node": {"remove": "ycfg:delete"}},
        ])
        self.assertEqual(results["ci"].config, {"node": {"keep": 1}})
        self.assert_failure({"ycfg": {"name": "dev"}, "value": None}, "null")

    def test_delete_missing_property_and_array_element_fail(self):
        for content in ({"missing": "ycfg:delete"}, {"array": ["ycfg:delete"]},
                        {"node": {"missing": "ycfg:delete"}}):
            with self.subTest(content=content):
                self.assert_failure({"ycfg": {"name": "dev"}, **content}, "dev")

    def test_override_replaces_whole_value_and_creates_missing_property(self):
        for replacement in ({"new": 2}, [3], "text", 4, False):
            with self.subTest(replacement=replacement):
                results = self.resolve_document([
                    {"ycfg": {"name": "dev"}, "existing": {"old": 1}},
                    {"ycfg": {"name": "ci", "extend": "dev"},
                     "existing": {"ycfg:override": replacement},
                     "absent": {"ycfg:override": replacement}},
                ])
                self.assertEqual(results["ci"].config,
                                 {"existing": replacement, "absent": replacement})

    def test_override_resolves_nested_imports_without_inherited_content(self):
        self.write_yaml("donor.yaml", ["BTC"])
        results = self.resolve_document([
            {"ycfg": {"name": "dev"}, "subscription": {"old": True}},
            {"ycfg": {"name": "ci", "extend": "dev"},
             "subscription": {"ycfg:override": {"symbols": "ycfg:import donor.yaml"}}},
        ])
        self.assertEqual(results["ci"].config, {"subscription": {"symbols": ["BTC"]}})

    def test_malformed_override_and_unknown_instruction_fail(self):
        for value in ({"ycfg:override": {}, "extra": 1}, "ycfg:unknown value",
                      {"ycfg:unknown": 1}, "ycfg:import"):
            with self.subTest(value=value):
                self.assert_failure({"ycfg": {"name": "dev"}, "value": value},
                                    "dev", "value")

    def test_generation_substitution_preserves_text_and_string_type(self):
        result = self.resolve_document({
            "ycfg": {"name": "dev"},
            "value": "{ycfg:gen-var COUNT}",
            "path": "{ycfg:gen-var ROOT}/{ycfg:gen-var NAME}/{ycfg:gen-var NAME}",
            "nested": [{"value": "{ycfg:gen-var EMPTY}"}],
            "runtime": "{ycfg:run-var UNSET}",
            "both": "{ycfg:gen-var ROOT}/{ycfg:run-var UNSET}",
        }, environment={"COUNT": "42", "ROOT": "/srv", "NAME": "data", "EMPTY": ""})["dev"]
        self.assertEqual(result.config,
                         {"value": "42", "path": "/srv/data/data", "nested": [{"value": ""}],
                          "runtime": "{ycfg:run-var UNSET}",
                          "both": "/srv/{ycfg:run-var UNSET}"})

    def test_generation_placeholders_in_section_and_property_imports(self):
        donor = self.write_yaml("library/donor.yaml", {"value": 7})
        result = self.resolve_document({
            "ycfg": {"name": "dev", "import": "{ycfg:gen-var LIB}/donor.yaml"},
            "other": "ycfg:import {ycfg:gen-var LIB}/donor.yaml:/value",
        }, environment={"LIB": str(donor.parent)})["dev"]
        self.assertEqual(result.config, {"value": 7, "other": 7})

    def test_substitution_does_not_interpret_inserted_text(self):
        for text in ("{ycfg:gen-var MISSING}", "ycfg:delete", "ycfg:import absent.yaml"):
            with self.subTest(text=text):
                result = self.resolve_document({
                    "ycfg": {"name": "dev"}, "value": "{ycfg:gen-var TEXT}",
                }, environment={"TEXT": text})["dev"]
                self.assertEqual(result.config, {"value": text})

    def test_missing_generation_variable_fails_with_context(self):
        self.assert_failure({"ycfg": {"name": "dev"},
                             "connection": {"token": "{ycfg:gen-var MISSING}"}},
                            "dev", "connection", "token", "MISSING")

    def test_malformed_placeholders_fail_in_authored_values(self):
        for value in ("{ycfg:gen-var}", "{ycfg:gen-var NAME", "{ycfg:run-var}",
                      "{ycfg:unknown NAME}"):
            with self.subTest(value=value):
                self.assert_failure({"ycfg": {"name": "dev"}, "value": value},
                                    "dev", "value")

    def test_runtime_placeholders_cannot_resolve_import_paths(self):
        for content in (
            {"ycfg": {"name": "dev", "import": "{ycfg:run-var LIB}/donor.yaml"}},
            {"ycfg": {"name": "dev"},
             "value": "ycfg:import {ycfg:run-var LIB}/donor.yaml"},
        ):
            with self.subTest(content=content):
                self.assert_failure(content, "dev", environment={"LIB": str(self.directory)})

    def test_default_environment_and_explicit_environment_are_distinct(self):
        content = {"ycfg": {"name": "dev"}, "value": "{ycfg:gen-var YCFG_TEST_VALUE}"}
        self.write_yaml(self.source.name, content)
        with patch.dict("os.environ", {"YCFG_TEST_VALUE": "from environment"}, clear=True):
            result = generate_configs(str(self.source))["dev"]
            self.assertEqual(result.config["value"], "from environment")
            self.assert_failure(content, "YCFG_TEST_VALUE", environment={})
            result = generate_configs(self.source, environment={"YCFG_TEST_VALUE": "explicit"})["dev"]
            self.assertEqual(result.config["value"], "explicit")

    def test_missing_import_reports_source_section_property_and_donor(self):
        self.assert_failure({"ycfg": {"name": "dev"},
                             "connection": "ycfg:import missing.yaml"},
                            "dev", "connection", "missing.yaml")

    def test_invalid_source_yaml_is_config_error(self):
        self.source.write_text('{"ycfg":', encoding="utf-8")
        with self.assertRaises(config_error) as caught:
            generate_configs(self.source, environment={})
        self.assertIn(self.source.name, str(caught.exception))

    def test_invalid_donor_yaml_reports_import_context(self):
        self.source.with_name("donor.yaml").write_text('{"missing":', encoding="utf-8")
        self.assert_failure({"ycfg": {"name": "dev"}, "value": "ycfg:import donor.yaml"},
                            "dev", "value", "donor.yaml")

    def test_missing_source_is_config_error(self):
        with self.assertRaises(config_error) as caught:
            generate_configs(self.source, environment={})
        self.assertIn(self.source.name, str(caught.exception))

    def test_resolution_does_not_write_or_change_files(self):
        self.write_yaml("donor.yaml", {"value": 1})
        self.write_yaml(self.source.name, {"ycfg": {"name": "dev", "import": "donor.yaml"}})
        before = {path: path.read_bytes() for path in self.directory.rglob("*") if path.is_file()}
        generate_configs(self.source, environment={})
        after = {path: path.read_bytes() for path in self.directory.rglob("*") if path.is_file()}
        self.assertEqual(after, before)


if __name__ == "__main__":
    unittest.main()
