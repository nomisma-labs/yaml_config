# Changelog

Notable changes to this repo. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org); how to release:
[docs/kit/build.md](docs/kit/build.md#versioning).

## [Unreleased]

## [0.2.0] - 2026-11-11

### Changed

- License project-owned code and documentation under MIT, retaining
  Alexey Tkachenko's copyright. Third-party dependencies retain their own licenses.
- Synchronize cmake_template v0.14.0 and record the repository license policy;
  the shared kit retains MIT independently of the project selection.
- Include the project license in installed artifacts alongside the applicable
  toolchain and dependency notices.
- Update internal dependency pins to `base v0.5.0`.

## [0.1.0] - 2026-11-11

### Added

- Initial template scaffold for the YAML application configuration framework.
- Parser comparison and isolated fkYAML conversion/module probes.
- YAML language and mapping guides, complete fixtures and application examples.
- Python codec, section resolver and `ycfggen.py`, including imports, inheritance,
  generation variables, metadata listing and atomic publication.
- CMake generation, collection, cleanup, ownership and deployment helpers.
- C++ named module with checked native fkYAML mapping, enums, containers, optional
  and defaulted members, config lookup, arguments, substitution and typed overrides.
- Version-pinned fkYAML compatibility headers and Python/C++ value parity tests.
- Runtime, CLI, workflow and source/installed-package consumer tests.
