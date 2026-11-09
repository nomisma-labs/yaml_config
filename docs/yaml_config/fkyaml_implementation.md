# Implementation details using fkYAML

> **Design for review — implementation pending.** This document specifies the
> adapters to implement after review. The standalone native-library probe is
> complete; the framework mapping macros and runtime are not.

This document maps the [Nomisma contract](yaml_cpp_mapping.md) onto **fkYAML 0.5.0**.
The [parser research](parser_research.md) records the recommendation, pinned
revision, observed limitations and module probe. The dependency is a pinned release
submodule under `external/fkyaml`; `<yaml_config/fkyaml.h>` includes its native
header and is also included by `config.h`. Textual third-party and macro headers
come before imports, in the global module fragment for module units.

The [config types design note](https://github.com/nomisma-labs/base/blob/master/docs/base/config_types_design.md)
explains the separation of common data and format mapping. `base` owns
`nom.config_types`; `nom.yaml_config` supplies YAML adapters and re-exports that
module. Common config types do not depend on fkYAML or the Python generator.

## Overview of fkYAML facilities

fkYAML uses `from_node` and `to_node` functions in a type's namespace, found by
argument-dependent lookup. An ordinary native conversion looks like this:

```cpp
#include <yaml_config/fkyaml.h>

import std;
import nom.yaml_config;

namespace demo
{
    using namespace nom::cfg_types;

    struct connection_config
    {
        cfg_string address;
        cfg_int timeout_s;
    };

    void from_node(const fkyaml::node& document, connection_config& value)
    {
        value.address = document.at("address").as_str();
        document.at("timeout_s").get_value_inplace(value.timeout_s);
    }

    void to_node(fkyaml::node& document, const connection_config& value)
    {
        document = fkyaml::node::mapping();
        document["address"] = value.address;
        document["timeout_s"] = value.timeout_s;
    }
}
```

Given a parsed mapping named `document`, the corresponding operations are:

```cpp
auto config = document.get_value<demo::connection_config>();
fkyaml::node serialized = config;
```

The native conversion example does not register the structure as a config member
or add per-member policies. Use `NOM_YCONFIG` for ordinary application structures.
It generates these two converters, registers the type, checks enabled member types,
and applies the presence and strict-value policies below. Use a repository-owned
macro helper; there is no dependency on nlohmann/json or toml11 macro internals.

| fkYAML facility | Behavior relevant to mapping |
|---|---|
| `get_value<T>()` | Creates a value, then invokes its converter |
| `get_value_inplace(existing)` | Passes the existing object to its converter; preserving defaults depends on the converter |
| `fkyaml::node(value)` | Converts a C++ value through `to_node` |
| `is_*()` and `as_*()` | Inspect a node category and access its native value |
| `deserialize_docs(text)` | Parses the stream; the framework must require exactly one document |
| `serialize(document)` | Emits YAML; the framework first validates its allowed value profile |

The native [conversion API](https://fktn-k.github.io/fkYAML/api/basic_node/get_value_inplace/)
and [converter dispatch](https://fktn-k.github.io/fkYAML/api/node_value_converter/from_node/)
provide these operations. Direct conversions to built-in numeric types are more
permissive than the Nomisma contract and must not substitute for checked adapters.

## Config-enabled types

Only enabled types may appear in a `NOM_YCONFIG` mapping. Check wrappers and
container elements recursively.

| Type | Supported representation |
|---|---|
| `nom::cfg_bool` (`bool`) | YAML boolean only |
| `nom::cfg_string` (`std::string`) | YAML string only |
| `nom::cfg_int` | YAML integer in the `std::int64_t` range |
| `nom::cfg_uint` | Nonnegative YAML integer in the full `std::uint64_t` range |
| `nom::cfg_double` | Integer or floating value convertible to finite `double` |
| Structure declared with `NOM_YCONFIG` | YAML mapping; the macro enables the structure |
| Enum declared with `NOM_YCONFIG_ENUM` | Explicitly mapped YAML string |
| `nom::cfg_vector<T>` (`std::vector<T>`) | Sequence of enabled values |
| `nom::cfg_map<T>` or `nom::cfg_unordered_map<T>` | String-keyed mapping of enabled values |

The aliases in `nom::cfg_types` name the same common types. Plain numeric members
such as `int`, `unsigned` and `double` are not enabled. The checked numeric types
remain aggregates with a public `value`, `get()` accessors and implicit conversion
by value to the underlying C++ type. Mapping adds no storage to those types.

`cfg_defaulted<T>` and optional members apply the presence policies below. Dates
and times have no native config value category: write them as strings and use an
explicit domain converter if needed. There is no implicit timezone conversion.

## Mapping and custom converters

Include `<yaml_config/config.h>`, import `nom.yaml_config`, and link
`nom::yaml_config`. Its public dependency on `nom::config_types` supplies the
common module. The installed package finds `base` and Boost and bundles its patched fkYAML
headers through `nom::yaml_config_fkyaml`; source and installed consumers use the
same target. Do not mix another fkYAML header provider into this build. Converter
declarations mention `fkyaml::node`, so all consumers need the same header definitions.

| Operation | Purpose |
|---|---|
| `document.get_value<Config>()` | Create and populate a config |
| `document.get_value_inplace(candidate)` | Populate an existing initialized candidate |
| `fkyaml::node document = config` | Serialize through the type's converter |

Custom converters belong beside their type:

```cpp
void from_node(const fkyaml::node& document, domain_type& value);
void to_node(fkyaml::node& document, const domain_type& value);
```

For renamed keys or a domain type represented by a scalar or sequence, use native
node access and these conversion functions. Enable the type before its first use
as a mapped member:

```cpp
template <>
inline constexpr bool nom::enable_config_type<domain_type> = true;
```

Enabling a type does not provide its converters or validate its representation.
Custom converters remain responsible for domain checks, strict scalar categories,
and preserving an existing destination where appropriate.

`NOM_YCONFIG_ENUM` generates native converters and enables the enum. Its exact
string matching, explicit aliases, first-name serialization and failure behavior
are specified in the [enum mapping guide](yaml_cpp_mapping.md#enum-mapping).
Check for a string node first. Assign the destination only after finding a valid
entry; reject unknown names and unmapped enum values, with no fallback enumerator.

Mapping failures propagate through `std::exception`-compatible exceptions.
Nomisma adapters add a config pointer when crossing each member or container
boundary, for example `/connections/1/timeout_s`. Native fkYAML parse errors carry
line/column information, but its value nodes do not supply nlohmann-style parent
tracking. Do not promise source locations for synthesized values or every custom
conversion error. The loader adds the loading stage and selected file context.

## Default and optional wrappers

Use an ordinary member for a required value, `cfg_defaulted<T>` for an omittable
value with a C++ default, and `cfg_optional<T>` (an alias of `std::optional<T>`)
for a value that may be absent.

`cfg_defaulted<T>` retains its public `value`, mutable/const `get()`, implicit
conversion to `T&`/`const T&`, and dereference/arrow operators. Initialize it as
`cfg_defaulted<cfg_int> timeout_s{5}` and use `*timeout_s > 0` or
`config.connection->address` without copying the wrapped value.

| Member | Missing key | Supplied non-null value | Supplied null |
|---|---|---|---|
| Ordinary `T` | Error, even with a C++ initializer | Map to `T` | Error |
| `cfg_defaulted<T>` | Preserve initialized value | Map into wrapped `T` | Error |
| `cfg_optional<T>` | Preserve initialized optional | Map into the contained `T` | Error |

An optional normally starts empty. If C++ initializes it with a value, omission
keeps that value. There is no text representation for clearing a preinitialized
optional: null, `~`, and implicit empty YAML values are rejected. A malformed
supplied value never selects a fallback default.

A default wrapper serializes as its underlying value. An engaged optional
serializes as its contained value; an empty optional omits the member. Consequently,
a C++ object whose preinitialized optional was later cleared cannot round-trip
that cleared state through this representation. This is the same omission contract
as `toml_config` and honors the choice to avoid null in configs.

Optional handling belongs to the structure-member helper. Optional elements inside
containers and `cfg_defaulted<std::optional<T>>` are unsupported; no global
specialization for all `std::optional<T>` is needed. Missing optional members are
not existing runtime `--set` targets.

## Object shape and nested defaults

Each `NOM_YCONFIG` converter requires a mapping and uses declared member names as
keys. Unknown ordinary keys are ignored by structure mapping; invalid YAML-profile
features are still rejected when parsing the document. Required keys must be
present, even if an initializer exists. A supplied nested mapping must satisfy
its own required members.

Initialize a private C++ candidate once. Populate nested members in place. If a
connection type defaults its timeout to `5`, while the parent initializes that
connection with `42`, supplying only the address must preserve `42`.
For an engaged optional, populate its existing contained object; for an empty
optional, construct a contained value before mapping.

Sequence and map inputs replace their corresponding containers; they do not merge
with C++ containers. Elements start from their own defaults. Walk elements through
the checked mapping helpers so numeric/boolean rules and nested diagnostic pointers
are retained, including for `std::vector<bool>`. Preserve native ADL conversion
for user types; do not introduce a second user-facing converter protocol.

Do not serialize defaults and merge them into the input before mapping: that
would hide required-key errors. In-place mapping can partially modify a candidate
before failing. Publish it only after complete mapping and root validation succeed.

## Value conversion

Inspect the source category before retrieving or converting its value. Native
fkYAML conversions between bool, integer and floating types are intentionally
more permissive. In particular, `cfg_bool` is a plain `bool` alias, so member and
container helpers must guard its boolean category explicitly rather than depend
on the native `get_value<bool>()` conversion.

`cfg_int` and `cfg_uint` reject booleans, floating values (including `1.0`), strings
and out-of-range integers. `cfg_uint` also rejects negatives. Use `is_uint()` and
`as_uint()` before signed access: fkYAML stores large unsigned values under its
integer category and `as_int()` rejects values above `INT64_MAX`.

`cfg_double` accepts integer and floating nodes, permits ordinary floating rounding,
and requires a finite result in both directions. String and boolean fields do not
coerce numbers. The shared profile rejects nonfinite nodes before mapping; numeric
adapters must also protect direct conversion of programmatically created nodes.

The [value profile](ycfg.md#yaml-values-and-portability) defines parser resolution.
For example, unquoted `012` and out-of-range numeric-looking text become strings
with the pinned parser; checked numeric mapping rejects those strings. The Python
codec must agree. Schema/type checking belongs here; application-specific ranges
and relationships belong to the root `validate()` protocol.

## Pinned compatibility patch

The upstream v0.5.0 checkout remains unchanged. Configuration creates a private
header tree with [fkyaml_numeric.patch](../../cmake/yaml_config/patches/fkyaml_numeric.patch),
guarded by SHA-256 checks of both original headers. Dependency updates require
reviewing and retesting the patch; an unexpected header fails configuration.
Installed consumers receive this same patched tree and the upstream license.

The patch accumulates negative decimals without an overflowing positive magnitude,
checks hexadecimal/octal accumulation before multiplication, and emits floating
values with `max_digits10` precision and the classic locale. These changes fix
`INT64_MIN` undefined behavior, unsigned boundary wraparound, and precision loss
in native serialization. Shared Python/C++ corpus tests and sanitizer regressions
cover the numeric boundaries. The framework additionally enforces its explicit
YAML 1.2 directive policy before parsing; fkYAML itself accepts future 1.x versions.
