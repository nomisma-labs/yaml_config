# YAML parser selection

Research date: **2026-11-09**. This is a design recommendation, not an implemented
configuration framework. The comparison uses the published releases below and
isolated probes, separate from the future `nom.yaml_config` API.

## Recommendation

Use **fkYAML 0.5.0** for C++. Its typed, owning `fkyaml::node` tree and native
`from_node` / `to_node` conversion hooks are the closest fit to the nlohmann/json
workflow used by `json_config`. `get_value_inplace()` can populate an existing
object, which lets the mapping preserve parent-supplied nested defaults. The
probe exercises these native hooks across a named-module boundary, including
an enum and full unsigned 64-bit round trips.

This recommendation prioritizes the config author and application developer APIs.
No performance benchmark or exhaustive YAML conformance test was run. Startup
configuration does not currently justify choosing a parser on throughput alone.

## Candidates

| Candidate / inspected release | Useful facilities | Cost for this framework | Assessment |
|---|---|---|---|
| [fkYAML 0.5.0](https://github.com/fktn-k/fkYAML/releases/tag/v0.5.0) | Header-only, owning typed nodes, STL containers, ADL converters, in-place conversion, serialization, unsigned 64-bit values | Scalar-resolution differences need a pinned profile; no native equivalent of nlohmann diagnostic parent pointers | Recommended; smallest gap from the existing application mapping model |
| [yaml-cpp 0.9.0](https://github.com/jbeder/yaml-cpp/releases/tag/yaml-cpp-0.9.0) | `YAML::Node`, `as<T>()`, `YAML::convert<T>`, source marks, emitter | Scalars retain text/tags rather than an integer/bool/string value category; native conversions accept quoted numeric/boolean text; duplicate-key checks and scalar resolution need framework code | Viable alternative, especially if full control of scalar resolution becomes more important than typed-node convenience |
| [rapidyaml 0.16.0](https://github.com/biojppm/rapidyaml/releases/tag/v0.16.0) | Tree/node references, explicit parsing and emission, customization through read/write operations | Text-oriented node values and buffer/arena ownership require more adaptation for an owning typed config tree | Keep as an alternative if measured parser cost becomes significant |

The candidate APIs are documented in the
[fkYAML API reference](https://fktn-k.github.io/fkYAML/api/basic_node/get_value_inplace/),
[yaml-cpp tutorial](https://github.com/jbeder/yaml-cpp/wiki/Tutorial), and
[rapidyaml quickstart](https://rapidyaml.readthedocs.io/v0.16.0/doxygen/group__doc__quickstart.html).
All three use the MIT license. Their published versions were checked with GitHub's
release API on the research date; this is not a promise to track future releases automatically.

## Observed scalar behavior

Probed against fkYAML commit `5cc0747068b94708aa0986cacd200aa74be4f9e8`
(tag `v0.5.0`) and yaml-cpp commit
`56e3bb550c91fd7005566f19c079cb7a503223cf` (tag `yaml-cpp-0.9.0`).

| Input | fkYAML 0.5.0 observation | Consequence |
|---|---|---|
| `true`, `1e3`, `0o12`, `0xFF` | Boolean, floating value `1000.0`, integers `10` and `255` | Native node type can drive checked mapping |
| `on`, `yes`, `2026-11-09` | Strings | No YAML 1.1 boolean words or implicit dates |
| `"12"` | String | Checked integer mapping must reject it |
| `012`, `-0xFF`, `0XFF`, `1_000` | Strings | Do not describe the resolver as an exact YAML 1.2 Core implementation; use canonical decimal spellings for numbers |
| `18446744073709551615` | Unsigned integer, preserved by serialization and reparse | `cfg_uint` can support its full underlying range |
| `18446744073709551616`, `-9223372036854775809`, `1e999` | Strings, quoted by serialization | Numeric mapping rejects them by type; a string field can accept them. This fallback must be reproduced in Python |
| `.inf`, `.nan` | Nonfinite floating values | Reject in the framework value profile, before application mapping |
| `null`, `key:` followed by a newline | Null node | Reject explicit and implicit null; optional absence uses an omitted key |
| Duplicate mapping key | Parse error | Retain rejection in Python and runtime loading |
| Two `---` documents | Two nodes from `deserialize_docs()` | Require exactly one document; never silently read only the first |
| Anchor, alias, explicit tag | Native metadata retained | Can reject these features before walking/using their values |

An additional edge case: native `deserialize("key:")` without a final newline
returned a string in the probe, whereas `"key:\n"` produced a mapping with a null
value. File parsing should append a terminating newline when absent before calling
fkYAML. The shared profile rejects the null value in either spelling. This
normalization needs regression coverage in implementation.

The numeric fallback is intentional upstream behavior, described in the
[release notes](https://fktn-k.github.io/fkYAML/home/releases/) and implemented in the
[pinned scalar parser](https://github.com/fktn-k/fkYAML/blob/v0.5.0/include/fkYAML/detail/input/scalar_parser.hpp).
It is a real tradeoff of this recommendation. Reconsider the parser or a small
upstream change if exact Core-schema resolution or rejection of overflowing
numeric-looking text as a parse error becomes a requirement.

The yaml-cpp probe found `as<int>()` accepts both `12` and `"12"`, `as<bool>()`
accepts both `true` and `"true"` as well as `yes` and `on`, and `012` converts to
integer `10`. Duplicate keys remain two mapping entries and lookup returned the
first. A stricter facade could change all of these policies, but would be an
additional conversion layer to maintain.

## Python side

Use **PyYAML 6.0.3**, with its event/composition APIs and a repository-owned codec.
The default `safe_load()` behavior is unsuitable: the local probe resolved
`on`/`yes` to booleans, `012` to `10`, dates to `datetime.date`, `1e3` to a string,
and duplicate keys to the last value. These disagree with the chosen C++ parser.

`yaml_codec.py` must consume unresolved scalar text and style (using `BaseLoader`
or parser events), enforce the framework profile, and resolve scalar types
explicitly. It must not construct arbitrary Python objects or use the default
implicit resolvers. [PyYAML's documented low-level APIs](https://pyyaml.org/wiki/PyYAMLDocumentation)
provide the required primitives. A shared fixture corpus must compare Python
values and C++ node categories, including serialization/reparse, before declaring
the codec complete. Pin the dependency in the generator's requirements file at
implementation time; the standard library has no YAML parser.

## Proposed authoring boundaries

Use one UTF-8 YAML document with string-keyed mappings, sequences, finite numbers,
booleans and strings. Reject nulls, duplicate keys, non-string keys, anchors,
aliases, explicit tags and merge keys. `ycfg` supplies the existing inheritance,
import and replacement operations; YAML graph semantics would add another merge
model and make independent section copies harder to reason about.

Sections use a root sequence, like jcfg, or a single mapping shorthand. No
`sections` envelope and no multi-document section syntax are needed. The
[YAML specification](https://yaml.org/spec/1.2.2/) describes the broader language;
the framework deliberately specifies a narrower, testable application profile.
These decisions are part of the documentation review.

## Evidence and remaining gates

The [standalone probe](../../tools/yaml_config/research/README.md) checks real ADL
converters, enum errors, preserved initializers, typed scalar boundaries and
unsigned serialization with a named module and `import std`. It passed with
GCC 16 and Clang 22 on 2026-11-09. This verifies the native facilities, not the
future `NOM_YCONFIG` macros, loader, or installed-package boundary.

Before implementation acceptance, require:

- A shared Python/C++ fixture corpus for accepted and rejected YAML, including
  scalar spelling, duplicate keys, tags/aliases, empty inputs and multiple documents.
- Checked numeric, enum, container, optional and nested-default mapping tests.
- Generator/build workflow and CLI parity with the reference frameworks.
- GCC/Clang builds and source/installed consumers; module export and dependency
  packaging must be checked with the final adapters, not inferred from the probe.

## Implementation findings

Framework parity and UBSan tests exposed three issues in the pinned release:
negative decimal accumulation overflows at `INT64_MIN`, large hexadecimal/octal
integers can wrap during signed accumulation, and native float emission uses only
six significant digits. The approved [compatibility overlay](fkyaml_implementation.md#pinned-compatibility-patch)
fixes these cases without modifying the upstream submodule. The recommendation
therefore applies to the pinned, patched header provider, not an arbitrary system
installation of fkYAML 0.5.0.
