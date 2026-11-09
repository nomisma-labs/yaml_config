# YAML framework status

Documentation and implementation are complete for the first version, pending the
user's implementation review. No release has been published. Documentation was
approved on **2026-11-09**; implementation followed in committed stages.

## Remaining work

- Review the implementation and reconcile any requested corrections with the docs.
- Release only after a separate release request.

## Completed implementation

- Template-derived repository, workspace registration and parser research.
- Eight framework guides, parser notes, three documented applications and 50 YAML
  source/donor/expected-output fixtures.
- Python value codec, section resolver and generator, including inheritance,
  imports, substitutions, deletion/replacement, metadata listing and atomic output.
- CMake generation, collection, ownership records, cleanup and installation.
- `nom.yaml_config`, re-exporting `base` config types, with native ADL mapping,
  checked numbers/booleans, enums, containers, nested defaults, optional omission
  and diagnostic pointers.
- Runtime arguments, location, substitution, ordered typed overrides, root
  validation and full/view/silent check modes.
- Source and installed-package consumers, including negative compilation checks
  for unsupported members, containers and wrapper combinations.
- Unchanged format-independent `cfg_*` Bash helpers.

## Approved design

- fkYAML 0.5.0 with the approved version-pinned
  [compatibility overlay](docs/yaml_config/fkyaml_implementation.md#pinned-compatibility-patch).
  The upstream submodule remains unchanged; source and installed consumers use
  the same patched headers. SHA-256 guards cover all five affected headers.
- The overlay fixes integer boundary overflow, float precision/locale, truncated
  flow collections, and string escaping/quoting. These findings and regressions
  are documented in [parser research](docs/yaml_config/parser_research.md).
- PyYAML 6.0.3 with explicit scalar resolution matching the pinned C++ profile.
  Some leading-zero or overflowing numeric-looking tokens are strings; checked
  numeric mapping rejects those strings. This is not an exact Core-schema claim.
- One UTF-8 document; string keys; reject duplicate keys, null, nonfinite numbers,
  anchors, aliases, explicit tags and merge keys. Missing optional/defaulted members
  preserve C++ initializers; empty optionals serialize by omitting their keys.
- `ycfg` sections use a root sequence or single mapping shorthand. No `sections`
  envelope or multi-document section convention.
- Canonical `.yaml` and `*_ycfg.yaml` discovery; explicit `.yml` paths work.
- Familiar `NOM_YCONFIG`, `NOM_YCONFIG_ENUM`, `nom::ycfg`, generator/CMake names,
  `cfg_*` helpers and `NOM_CONFIG*` settings. A project chooses one framework.
- RFC 6901 token syntax for fragment imports and runtime overrides. Generator
  listings and private build records retain JSON metadata independently of YAML.

## Validation — 2026-11-09

- Fresh `./bootstrap.sh complete` with GCC 16: all **22 CTest entries pass**;
  10 bootstrap checks pass, none fail, dependency-update reporting is skipped.
- Fresh `./bootstrap.sh clang` with Clang 22: all **22 CTest entries pass**;
  11 bootstrap checks pass, none fail, dependency-update reporting is skipped.
- The suites include 149 Python tests, generated-config checks, C++ mapping/runtime
  tests, and source/installed consumers with five compile-rejection cases each.
- **233** shared corpus/documentation cases agree between Python and C++, including
  both serializers, malformed inputs, numeric boundaries and escaped strings.
- Clang AddressSanitizer, UBSan and LeakSanitizer: all **7 unit CTest entries pass**
  with applications disabled. LeakSanitizer cannot run under the tracing sandbox;
  the successful run used the same binaries outside that sandbox, with leak
  detection enabled. Invalid-input parity checks require normal caught exceptions
  and reject sanitizer diagnostics/crashes.
- SPDX, formatting, naming, Python syntax/imports and local documentation links pass.
  The Bash helper is byte-identical to the JSON/TOML versions; `git diff --check` passes.
- Dependency-update discovery did not check `base` or fkYAML; the pinned versions
  above were tested. GitHub Actions is disabled; no remote CI result is claimed.

## Baselines

- `cmake_template` v0.13.0: `ccd09413e6a6421dd1b6b97873c1be11bc44e06e`.
- `json_config`: `d2db3130349861f88236ac1b00cc767cbc6e3b91`.
- `toml_config` v0.1.0: `1ca0bb0cb3aa4f21d4c61ea53e2bb3427a45dcea`.
- `base` v0.4.0: `51dcfd4be7b59ed7147557d973e0363e2a4b23b7`.
- fkYAML v0.5.0: `5cc0747068b94708aa0986cacd200aa74be4f9e8`.
