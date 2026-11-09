# YAML framework plan

Documentation approved on **2026-11-09**. Implementation is in progress. The new repository, template scaffold and isolated parser research are
complete; there is no released `yaml_config` library or working generator yet.

## Completed preparation

- Created private `nomisma-labs/yaml_config` from `cmake_template` v0.13.0 and
  initialized it with the template tool. `.template_ref` records
  `ccd09413e6a6421dd1b6b97873c1be11bc44e06e`.
- Registered `research/yaml_config` in the workspace manifests and editor folders.
- Validated the empty scaffold with `./bootstrap.sh`: 19 checks passed, no failures;
  five test runs skipped because the scaffold has no framework tests.
- Compared fkYAML 0.5.0, yaml-cpp 0.9.0 and rapidyaml 0.16.0 using primary sources.
  Recommended fkYAML; GCC 16 and Clang 22 native conversion/module probes passed.
- Ported the eight reference guides, supporting app/tool/consumer READMEs and
  50 config source/donor/expected-output fixtures. Added a separate parser research
  note. The examples specify target behavior; their applications are not implemented.
- Copied the format-independent `config_path.sh` unchanged.

## Documentation review

Review [the guide index](docs/yaml_config/README.md), especially
[parser selection](docs/yaml_config/parser_research.md) and the
[shared YAML profile](docs/yaml_config/ycfg.md#yaml-values-and-portability).

Approved design decisions:

1. fkYAML 0.5.0 with native ADL conversion, `get_value_inplace()` and full uint64
   support; PyYAML 6.0.3 with explicit compatible scalar resolution in Python.
   The native parser treats some leading-zero and overflowing numeric-looking text as
   strings. Checked numeric members reject those strings. This behavior is a
   documented tradeoff, not an exact YAML Core-schema claim.
2. Root sequence of section mappings, or one mapping shorthand, with `ycfg`
   metadata. No `sections` envelope and no multi-document section syntax.
3. One UTF-8 document; string keys; reject duplicate keys, null, nonfinite numbers,
   anchors, aliases, explicit tags and merge keys. These restrictions keep the
   existing tree/merge model. Retain initialized optional/defaulted members on
   omission; omit empty optionals when serializing.
4. Canonical `.yaml` files and `*_ycfg.yaml` discovery. Explicit paths can use
   `.yml`; automatic lookup/discovery does not add a second suffix convention.
5. Preserve names and conventions: `NOM_YCONFIG`, `NOM_YCONFIG_ENUM`, `nom::ycfg`,
   `ycfg.py`, `ycfggen.py`, `ycfg_build.py`, `nom_ycfg_generate`, `nom_ycfg_test`;
   keep `cfg_*` Bash helpers and `NOM_CONFIG*` settings unchanged. A project chooses
   one config framework; coexistence is outside the design.
6. Config pointers retain RFC 6901 token syntax for fragment imports and `--set`.
   Generator `--list` and private build records remain JSON metadata, independent
   of YAML config values.

The user approved implementation after documentation review. Commit each significant
stage and stop for a final implementation review. The user approved a version-pinned fkYAML compatibility patch after UBSan
exposed signed overflow in integer parsing.

## Implementation sequence

1. **Codec and Python generator.** Add pinned dependencies, `yaml_codec.py`,
   `ycfg.py` and `ycfggen.py`. First establish a shared Python/C++ value corpus;
   then port resolution, discovery, metadata listing, output containment and
   atomic per-file publication. Verify every documented example against its
   expected output. The C++ codec fixture runner may be isolated at this stage.
2. **Build integration.** Port `ycfg_build.py`, `nom_ycfg_generate`,
   `nom_ycfg_test`, output planning, ownership/success records, config targets,
   cleanup and installation checks. Respect the shared kit's project options.
3. **C++ mapping.** Add `base` and fkYAML dependencies, module/header boundaries,
   checked scalar/container conversion, structure and enum macros, optional/default
   policies, nested diagnostics and custom ADL converters. Cover compile-time
   rejection of unsupported types and in-place preservation of parent defaults.
4. **Runtime and examples.** Port arguments, location, runtime substitution,
   ordered string-first overrides, typed loading, root validation and check modes;
   implement all three documented applications and source/installed consumers.
5. **Acceptance and review.** Fresh GCC/Clang validation, focused sanitizer checks
   where useful, generator/build/CLI tests, package consumers, naming/format/header
   checks, example regeneration and doc reconciliation. Report exact evidence and
   remaining limitations, then stop for implementation review. Do not release
   until separately requested.

## References

- `json_config` baseline: `d2db3130349861f88236ac1b00cc767cbc6e3b91`.
- `toml_config` baseline: v0.1.0, `1ca0bb0cb3aa4f21d4c61ea53e2bb3427a45dcea`.
- Proposed `base` baseline: v0.4.0, `51dcfd4be7b59ed7147557d973e0363e2a4b23b7`.
- Proposed fkYAML baseline: v0.5.0, `5cc0747068b94708aa0986cacd200aa74be4f9e8`.
- JSON-specific future enhancements are not part of this port.

## Documentation validation

- All 50 YAML fixtures match the transformed JSON reference values and pass native
  fkYAML parsing, serialization and reparsing.
- Local documentation links/anchors and YAML fenced examples pass inspection.
- SPDX, formatting, Python syntax/imports, Bash syntax, copied-helper identity and
  `git diff --check` pass. The research probe passes with GCC 16 and Clang 22.
- GitHub Actions is disabled for the new repository; no remote CI result is claimed.

## Implementation progress

- Python codec, section resolver and generator implemented; 48 engine/value tests
  and 43 CLI/publication tests pass, including all documented expected outputs.
- Shared profile corpus added; C++ parity and sanitizer checks are next with the approved parser
  compatibility patch. The Python milestone does not claim cross-parser parity.
- Build helpers, output ownership/success records, config targets, cleanup and
  installation checks are implemented; all 27 workflow tests pass.
- Fresh `./bootstrap.sh complete` passes all 3 current CTest entries (118 Python
  tests in total) and repository checks. Dependency-update reporting skips fkYAML
  because upstream releases use a `release/*` branch. C++ runtime/mapping remain pending.
