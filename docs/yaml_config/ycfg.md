# ycfg - a structural configuration language

> **Design for review — implementation pending.** APIs, commands and tests below
> specify the intended behavior. See [the implementation plan](../../TODO.md) for status.

## Objective
Nomisma **ycfg** language defines application configuration using YAML.
The purpose of this language is to add structural features to YAML configs, i.e. ability to compose a single valid config from several fragments. For those who are familiar with OOP this reminds "inheritance" and "aggregation". So instead of redefining potentially complex multi-component config for different purposes we define "base" config with some default parameters and then re-use it by extending it in "derived" configs (inheritance) or by defining common parameters in "donor" config and including parts or whole "donor" into an "acceptor" config (aggregation). Those are main structural mechanisms. Apart from it there would be "adjustment" instructions that controls how these extension (aka inheritance) or aggregation (aka import or inclusion) happens.
Another way of looking at ycfg - a "stackable" or "layered" configuration with each layer introducing or refining previous layers config properties.

## Basic conventions
**ycfg** files by convention have `*_ycfg.yaml` file suffix. Any ycfg file is a well formed YAML file so it can be processed with regular YAML tools (e.g. pretty printers etc.) The name of the ycfg file will have its special meaning when we come to integration with Nomisma build system, but in this guide we can use any name (usually just `app_ycfg.yaml`). Nomisma platform provides a tool to generate well formed YAML configs from ycfg so they can be used by Nomisma apps in different cases.


The canonical extension is `.yaml`; discovery searches for `*_ycfg.yaml`, and
runtime lookup uses `<key>.yaml`. Explicit filenames (including imports and
`--source`/`--config`) may use `.yml`, but there is no automatic `.yml` fallback.
The generator uses Python 3.11+ and PyYAML 6.0.3 through the framework codec.
The framework selects the [YAML value profile](#yaml-values-and-portability)
below for both generation and runtime loading.

## Motivating example
Just to make it more illustrative let's consider `md_capture` app that have 2 configs - for dev and prod environments (shown schematically):
```
dev:
    log.level = debug
    connection
        snapshot.address = wss://ws-api.binance.com:443/ws-api/v3
        address = wss://stream-sbe.binance.com:9443/ws
    subscription
        symbols = btcusdt
        channels = trade, depth
    md_log.file = binance-spot-%Y%m%d-%H.md

prod:
    log
        level = warn
        file = log/%Y%m%d/md_capture.log
    connection
        snapshot.address = wss://ws-api.binance.com:443/ws-api/v3
        address = wss://stream-sbe.binance.com:9443/ws
    subscription
        symbols = btcusdt, ethusdt
        channels = trade, depth
    md_log.file = md/%Y%m%d/binance-spot-%Y%m%d-%H.md
```
As you see even in this small config there are repeating and unique parameters and values. There are multiple ways how we can abstract them out. But first - why would we want to do it?
Here are the cases:
1. A trader/quant maintains a list of required symbols for MD capture. He wants to focus on this list only and not to get into details of app configuration. For that he creates a file `binance-spot-md-capture-symbols.yaml` and adds the required symbols there. On the other hand a dev, while testing the MD capture app, wants to focus on specific list of symbols (perhaps only one symbol for simplicity).
2. An infra team monitors the connection parameters of Binance MD end point. They learned that Binance migrated to a new infra with different DNS address. Infra team wants to apply this new address for all components homogenously - for that they maintain a single `binance-spot-connection.yaml` config which contains actual connectivity parameters, so they don't need to update configs of 20+ apps that connect to Binance market data end point. However one trader learned that one particular subscription channel that he subscribes (e.g. liquidation info) was not migrated properly by Binance, so he wants his app to use old connectivity parameters - so he applies an exception to his app config.

One possible way of defining a coherent config for this app and use cases is to:
1. Create base config which would be also a dev config, e.g. `md_capture.yaml` with complete dev config;
2. Create `md_capture_ycfg.yaml` which in `base` case imports `md_capture.yaml`;
3. Add `ci` section for integration environment that inherits base config and imports `binance-spot-connection.yaml`. It also updates parameters `log` and `md_log` to standard values for infra deployment;
4. Add `prod` section for main production deployment and inherit it from `ci`. Update `subscription.symbols` parameter - by including it from `binance-spot-md-capture-symbols.yaml`;
5. Add `prod-liquidation` section to collect liquidation data, inherit it from `prod` and update `connection` and `subscription` parameter sections.

As you see, by defining configuration this way we allowed several teams (dev, QA, devops/infra, researchers, traders) to collaborate and not to step on each other toes while configuring and using md_capture app.

## Structure of ycfg file

A **ycfg** file is one YAML document whose root is a nonempty sequence of
section mappings, or a single section mapping. The shorthand is equivalent to
a sequence containing that one section. Each section has a `ycfg` metadata
mapping and application properties alongside it. YAML natively supports a root
sequence, so there is no `sections` envelope. Multiple `---` documents are not
an alternative section syntax.

```yaml
- ycfg:
    name: base
    gen_file: app.yaml
  log:
    level: debug
- ycfg:
    name: prod
    extend: base
  log:
    level: warn
```

The single-section form is:

```yaml
ycfg:
  name: dev
  gen_file: app.yaml
log:
  level: debug
```

The section-root `ycfg` property is reserved and excluded from generated
application YAML. Nested keys named `ycfg` are ordinary application data.
Metadata accepts exactly `name`, `extend`, `import`, `generate`, and `gen_file`;
unknown keys and wrong types fail. There is no implicit version or file inheritance.

In ycfg-authored keys and values, the `ycfg:` marker is reserved for instructions
and placeholders; literal uses and unknown/malformed markers fail. Imported
ordinary YAML stays literal during generation. Quote instruction/placeholder
strings, especially ones beginning with `{`, which otherwise denote a flow mapping.

See [app_ycfg.yaml](examples/structure_config/app_ycfg.yaml) for an example with three
sections: `common`, `ci`, and `prod`. Each section defines an application config,
although writing its output file may be suppressed. A section usually represents
an environment (e.g. ci, prod) or a use case (e.g. sim, perf).

The same folder contains [app.yaml](examples/structure_config/app.yaml), the default
application config, and [symbol.yaml](examples/structure_config/symbol.yaml), the
symbol list imported by `prod`.

The structure follows these rules:

1. **Section identity and order.** Each section has a unique `ycfg.name`, including
   in the single-mapping form. The first section is the root and has no `ycfg.extend`.
   Its name can be any section name, such as `dev`, `sim`, `common`, or `base`;
   Every other section names exactly one parent through `ycfg.extend`, a string
   referring to an earlier section in the same file. All sections therefore derive
   directly or indirectly from the root; forward references and inheritance cycles
   are not allowed.
2. **Merge precedence.** Start with the parent's resolved application config, or
   an empty mapping for the root. `ycfg.import` accepts a single filename string or
   an array of filename strings; `"app.yaml"` is equivalent to `["app.yaml"]`.
   Merge those files in their listed order, then merge the section's local
   application properties. Omitting `ycfg.import` means no section-level imports.
   Mappings merge recursively; arrays and other values replace earlier values.
   Later values take precedence.
3. **Metadata inheritance.** Application data is inherited separately from
   metadata. `ycfg.name`, `ycfg.extend`, `ycfg.import`, and `ycfg.generate` describe
   only the current section and are not inherited. An explicit `ycfg.gen_file`
   selects its output filename; when omitted in a derived section, the filename
   is inherited from its parent. See
   [output filenames](#output-filenames-default-inherited-and-explicit) for the
   default naming rule. `ycfg.name`, `ycfg.extend`, and `ycfg.gen_file` are literal
   strings and do not support placeholders.
4. **Property imports.** A ycfg-authored property value such as
   `"ycfg:import symbol.yaml"` imports YAML data at that property before merging
   it into the inherited/imported configuration. Imported ordinary YAML remains
   literal data during generation. [Fragment imports](#import-a-fragment-of-a-yaml-file)
   select a value within a donor file.
5. **Config generation.** `ycfg.generate` is a boolean that defaults to `true` for
   each section. Setting it to `false` suppresses that section's output file, while
   its resolved application data and output filename remain available to children.
   Its imports and instructions are evaluated when it is an ancestor of a selected
   section. Children generate their own files by default, even when their parent
   has `ycfg.generate` set to `false`.

Validate metadata and inheritance for every section. Resolve imports and
instructions only for selected sections and their ancestors; unrelated sections
do not require their donor files or generation variables to be available.

In the example, `common` imports `app.yaml` and sets `log.level` to `debug`, but
`ycfg.generate` is `false`, so it produces no output file. The `ci` section changes
the level to `warn` and inherits the output filename `app.yaml`.
The `prod` section inherits from `ci`, selects `prod.yaml` as its output filename,
and imports the symbol list into `subscription.symbols`. Neither child specifies
`ycfg.generate`, so both generate their output files.

A common, optional layout places `<app>_ycfg.yaml` and a complete application
config, `<app>.yaml`, in `<app>/config`. The root section can import that YAML
file in full and use the same output filename. Source files and generated files
live in different locations: the build collects source configs under
`build/<preset>/config/`, and generated configs belong under
`build/<preset>/genconfig/`. When the source YAML already contains the desired
development configuration, the app can use it directly without generation.

## YAML values and portability

The framework uses YAML 1.2 syntax with an application profile pinned to fkYAML
0.5.0 scalar resolution. It is not a claim to implement every YAML feature or
exactly the Core schema. The [parser comparison](parser_research.md) explains
this choice and records the tested edge cases. The Python codec and C++ loader
must enforce the same profile for sources, donors, runtime files and emission.

- Input is UTF-8, optionally with one leading UTF-8 BOM, and contains exactly one
  document. A leading `---` or trailing `...` is allowed; an empty stream or a
  second document is rejected. `%YAML 1.2` is optional; other version directives
  are rejected. Append a final newline when absent before passing text to fkYAML.
- Mappings require string keys and reject duplicate names at every depth.
  Sequence or numeric keys are errors; quote keys such as `"true"` or `"12"`
  when their spelling would otherwise resolve to a non-string. Keys are compared
  after YAML string decoding. Mapping order is not semantically significant.
- Supported values are mappings, sequences, strings, booleans, integers from
  `-9223372036854775808` through `18446744073709551615`, and finite binary64
  floating values. `cfg_int` and `cfg_uint` apply their own narrower signed or
  nonnegative constraints. No implicit date/time type is introduced.
- Null (`null`, `Null`, `NULL`, `~`, or an empty value), nonfinite floats, anchors,
  aliases, explicit node tags and merge keys (`<<`) are rejected throughout the
  document, including ordinary donor data. Unused tag directives do not enable tagged values.
  Use ycfg imports/inheritance for reuse, omission for optional absence, `""`
  for an empty string, `[]` for an empty sequence and `{}` for an empty mapping.
- Block and flow collections, quoted/plain strings, literal/folded block strings
  and comments are authoring choices. The generator preserves parsed values and
  sequence order; it does not preserve comments, formatting or map-key order.

Scalar resolution follows the pinned parser, including these distinctions:

| YAML spelling | Resolved value |
|---|---|
| `true`, `True`, `TRUE`; `false`, `False`, `FALSE` | Boolean |
| `yes`, `no`, `on`, `off`, `2026-11-09` | String |
| `12`, `-12`, `+12`, `0`, `0o12`, `0xFF` | Integer (canonical decimal is recommended) |
| `1.0`, `.5`, `1e3` | Floating value |
| `012`, `1_000`, `0XFF`, `-0xFF` | String, not an integer |
| `"12"`, `'true'`, `"null"`, block string content | String, regardless of its contents |
| Integer-looking text outside the supported range; `1e999` | String fallback, as observed with fkYAML; numeric C++ fields reject it |
| `.inf`, `-.inf`, `.nan` and their recognized case variants | Native nonfinite values, rejected by the profile |

For other scalar spellings the pinned resolver is the compatibility baseline;
implementation must extend the shared Python/C++ fixtures before accepting them.
A parser upgrade that changes a scalar category is a format-contract change and
requires review. Quote identifiers and large numeric strings deliberately. A
numeric-looking string must stay a string after emission and reparse.

The ordinary application root, every resolved section and each whole-file section
import must be a mapping. Fragment donors may have any supported root value,
including a sequence or scalar. An empty mapping serializes as `{}\n`; an empty
file is invalid. Explicit null cannot clear an initialized optional; omitted keys
retain C++ initializers, normally empty for `cfg_optional`.

Placeholders are string substitutions, not YAML source interpolation. Both
`'{ycfg:gen-var NAME}'` and `"{ycfg:run-var NAME}"` are string placeholders after
YAML parsing. Substituted text remains a string and is never reparsed as a number,
boolean or collection. Quote a placeholder that begins with `{` so YAML does not
interpret it as a flow mapping. Comments are not substituted.

## Define basic ycfg mechanisms

Each example folder contains a ycfg source, any YAML files it imports, and the
expected application YAML under `genconfig/`. Examples with several sections group
their expected outputs under `genconfig/<section>/`, so sections can share an output
filename. These directories identify the results being illustrated; the build or
generator invocation selects the actual output location.

The examples introduce the mechanisms in this order:

1. [In-place config with an explicit output name](#in-place-config-with-an-explicit-output-name)
1. [Import a YAML config](#import-a-yaml-config)
1. [Import a fragment of a YAML file](#import-a-fragment-of-a-yaml-file)
1. [Output filenames: default, inherited and explicit](#output-filenames-default-inherited-and-explicit)
1. [Extend a section](#extend-a-section)
1. [Merge inherited and local parameters](#merge-inherited-and-local-parameters)
1. [Merge several YAML files into an extended section](#merge-several-yaml-files-into-an-extended-section)
1. [Delete a key and override a value](#delete-a-key-and-override-a-value)
1. [Override with imported content](#override-with-imported-content)
1. [Illustrate all basic mechanisms in one example](#complete-example)

### In-place config with an explicit output name

The [app_ycfg.yaml](examples/mech_01_config/app_ycfg.yaml) example defines a complete
application config directly in a single section. All application values are
written in place alongside the section's `ycfg` metadata.

- `ycfg.name` identifies the root section as `dev`.
- `ycfg.gen_file` names the output file `app.yaml`.
- `log` and `subscription` contain the application settings.

The source file uses the single-mapping form. Wrapping it in a one-element array
would have the same meaning. Resolving the section produces one YAML mapping
containing the application settings, with the `ycfg` metadata removed.
`ycfg.generate` is omitted, so it defaults to `true`. The expected output is shown in
[genconfig/app.yaml](examples/mech_01_config/genconfig/app.yaml).

### Import a YAML config

In [app_ycfg.yaml](examples/mech_02_config/app_ycfg.yaml), the single `dev` section
has `ycfg.import` set to `"app.yaml"`. This is equivalent to `["app.yaml"]`; an
array supports merging several whole YAML mappings in order. Relative import paths
are resolved from the ycfg source file's directory, so the input is the adjacent
[app.yaml](examples/mech_02_config/app.yaml).

The section has no local application properties. Its expected output,
[genconfig/app.yaml](examples/mech_02_config/genconfig/app.yaml), therefore contains
the same application data as the imported file. The source and output share a
filename but live in different locations. The source is also a complete config
that the application can use directly.

### Import a fragment of a YAML file

[app_ycfg.yaml](examples/mech_03_config/app_ycfg.yaml) imports two arrays from
[markets.yaml](examples/mech_03_config/markets.yaml), a donor containing separate
`spot` and `perps` settings:

| Destination property | Import instruction | Selected value |
|---|---|---|
| `subscription.symbols` | `ycfg:import markets.yaml:/spot/symbols` | The spot symbol array |
| `subscription.channels` | `ycfg:import markets.yaml:/spot/channels` | The spot channel array |

In `ycfg:import markets.yaml:/spot/symbols`, the file is `markets.yaml` and the
config pointer is `/spot/symbols`. The colon separates the filename from the pointer,
which retains its leading `/`. Without a colon, as in `ycfg:import markets.yaml`,
the whole document is imported. As in the previous example, the donor file is
located relative to the ycfg source.

After expanding generation placeholders, split the import argument (the text after
`ycfg:import`) at its first `:`. The colon is reserved and cannot appear in donor
file paths; any later colons belong to the config pointer.

The config pointer applies [JSON Pointer (RFC 6901)](https://www.rfc-editor.org/rfc/rfc6901)
token syntax to the parsed YAML tree. This is an addressing convention, not a
conversion to JSON. The empty pointer selects the entire document; `/` selects
an empty-string key. Dots are literal key characters. Array indexes must be `0`
or a nonzero digit followed by digits; leading zeroes, negatives and `-` fail.
Invalid `~` escapes fail.
`/spot/symbols/0` selects the first array element. In property names, `~1`
represents `/` and `~0` represents `~`. A missing member or invalid array index
is an error. The selected YAML value retains its type: an array stays an array,
and a string stays a string.

The selected arrays become the property values before the local section is
merged. The unrelated `perps` settings do not enter the application config.
See [genconfig/app.yaml](examples/mech_03_config/genconfig/app.yaml).

Fragment imports are also useful for keeping generation-time values in a separate
YAML map. In the same example, [values.yaml](examples/mech_03_config/values.yaml)
holds `log_level` and `timeout_s`. `ycfg:import values.yaml:/log_level` supplies
`log.level`, while `ycfg:import values.yaml:/timeout_s` supplies `connection.timeout_s`.
The generator copies these values into the output as a string and a number,
respectively, using the same fragment-import mechanism.

Imported ordinary YAML is not interpreted by the generator. Runtime environment
placeholders are resolved when the application loads the config.

### Output filenames: default, inherited and explicit

[app_ycfg.yaml](examples/mech_04_config/app_ycfg.yaml) illustrates three naming
cases. The default filename comes from the source filename: replace the
`_ycfg.yaml` suffix with `.yaml`. Thus `app_ycfg.yaml` defaults to `app.yaml`.
For an explicit source filename without that suffix, keep its basename unchanged.

| Section | Filename rule | Expected output |
|---|---|---|
| `dev` | No `ycfg.gen_file`; use the source-derived default | [dev/app.yaml](examples/mech_04_config/genconfig/dev/app.yaml) |
| `ci` | No `ycfg.gen_file`; inherit the parent's filename | [ci/app.yaml](examples/mech_04_config/genconfig/ci/app.yaml) |
| `prod` | Explicit `ycfg.gen_file` is `capture.yaml` | [prod/capture.yaml](examples/mech_04_config/genconfig/prod/capture.yaml) |

An explicit filename takes precedence over an inherited filename. Importing a
file supplies application data; its filename does not change the section's output
name. The `ci` and `prod` outputs here have the same application data even though
their filenames differ.

### Extend a section

In [app_ycfg.yaml](examples/mech_05_config/app_ycfg.yaml), `ci` extends `base`,
and `prod` extends `ci`. Both derived sections contain only `ycfg` metadata.
Their application settings and output filenames are inherited unchanged.

Compare [base/app.yaml](examples/mech_05_config/genconfig/base/app.yaml),
[ci/app.yaml](examples/mech_05_config/genconfig/ci/app.yaml), and
[prod/app.yaml](examples/mech_05_config/genconfig/prod/app.yaml): all three have the
same contents. Each derived section has exactly one parent, and that parent appears
earlier in the source array.

### Merge inherited and local parameters

[app_ycfg.yaml](examples/mech_06_config/app_ycfg.yaml) starts with a `base` config
and changes selected settings in `ci`. Compare
[base/app.yaml](examples/mech_06_config/genconfig/base/app.yaml) with
[ci/app.yaml](examples/mech_06_config/genconfig/ci/app.yaml):

- `log.level` changes from `debug` to `warn`.
- `log.file.name` changes to `ci.log`, while the inherited `rotationSize` remains.
- `connection.timeout_s` changes to `20`, while the inherited address remains.
- `subscription.symbols` becomes `["ethusdt"]`. The inherited symbol array is
  replaced in full; `subscription.channels` remains unchanged.

Object properties merge recursively. At every other value boundary, the later
value replaces the earlier one. Resolving `ci` leaves the resolved `base` config
unchanged, so other sections can still inherit the original values.

### Merge several YAML files into an extended section

In [app_ycfg.yaml](examples/mech_07_config/app_ycfg.yaml), `ci` extends `base`,
then imports [connection.yaml](examples/mech_07_config/connection.yaml) and
[deployment.yaml](examples/mech_07_config/deployment.yaml), in that order.
Local properties are applied last.

The value of `connection.timeout_s` shows the precedence:

| Step | Value |
|---|---|
| Inherit `base` | `5` |
| Merge `connection.yaml` | `10` |
| Merge `deployment.yaml` | `20` |
| Merge local properties | `30` |

The final config keeps the address from `connection.yaml`, the log filename from
`deployment.yaml`, and the local log level `error`. Other inherited values remain.
See [base/app.yaml](examples/mech_07_config/genconfig/base/app.yaml) and
[ci/app.yaml](examples/mech_07_config/genconfig/ci/app.yaml).

This is still single inheritance: `ci` has one parent section. The import list
adds ordinary YAML data sources to that inherited configuration. Consider that
multiple import is a powerful tool that can easily confuse both the writer and
the reader of the config, especially when many people collaborate on it. Typical
use case for this feature is building a config for multiple environments (e.g.
dev, research, ci, prod) where different people are responsible for different
parts of the app config. So if the config has such logical separation, then
multiple import is a useable tool. However if the config is not semantically
structured, then multiple import can create a mess.

### Delete a key and override a value

[app_ycfg.yaml](examples/mech_08_config/app_ycfg.yaml) applies two instructions
while merging `ci` into its inherited application config:

- `log.file` has the value `"ycfg:delete"`, removing that property. The inherited
  `log.level` remains.
- `subscription` contains a `ycfg:override` wrapper whose value is the complete
  replacement mapping. The inherited `snapshot` property disappears because the
  replacement contains only `symbols` and `channels`.

Compare [base/app.yaml](examples/mech_08_config/genconfig/base/app.yaml) with
[ci/app.yaml](examples/mech_08_config/genconfig/ci/app.yaml).

`ycfg:delete` operates on an mapping property. Deleting a missing property is an
error. YAML null is rejected by the value profile; it does not mean deletion.

An override wrapper contains exactly one property, `ycfg:override`. Its value
replaces the target in full, without recursive merging with the previous value.
It can be a mapping, sequence, or supported scalar; if the target property is absent,
the override creates it. Neither the delete instruction nor the override wrapper
is written to the generated config.

### Override with imported content

[app_ycfg.yaml](examples/mech_09_config/app_ycfg.yaml) uses an import instruction
as the value inside a `ycfg:override` wrapper:

| Section | Replacement source | Expected output |
|---|---|---|
| `base` | Initial in-place settings | [base/app.yaml](examples/mech_09_config/genconfig/base/app.yaml) |
| `ci` | `ycfg:import subscription.yaml` | [ci/app.yaml](examples/mech_09_config/genconfig/ci/app.yaml) |
| `prod` | `ycfg:import markets.yaml:/perps` | [prod/app.yaml](examples/mech_09_config/genconfig/prod/app.yaml) |

The `ci` replacement comes from the complete
[subscription.yaml](examples/mech_09_config/subscription.yaml) document. The
`prod` replacement comes from the `perps` mapping inside
[markets.yaml](examples/mech_09_config/markets.yaml).

First resolve the import, including its pointer if present. Then assign the
result as the complete `subscription` value. The inherited `snapshot` property
does not survive either replacement. Imports inside an in-place replacement
mapping are also resolved before that mapping is assigned.

### Complete example

[md_capture_ycfg.yaml](../../apps/md_capture/config/md_capture_ycfg.yaml) brings the
mechanisms together for the [motivating example](#motivating-example).
The planned [md_capture application](../../apps/md_capture/README.md) loads these
configs into C++, validates them, and illustrates the resulting capture settings.
The application and this guide share the inputs in its `config/` directory;
the expected generated outputs remain linked below.

The input files separate the responsibilities:

- [md_capture.yaml](../../apps/md_capture/config/md_capture.yaml) contains a complete
  development config with literal values. It can be used without generation.
- [connection.yaml](../../apps/md_capture/config/connection.yaml) supplies the current
  connection settings shared by deployed variants.
- [deployment.yaml](../../apps/md_capture/config/deployment.yaml) supplies deployment
  logging settings and the market-data output path.
- [symbols.yaml](../../apps/md_capture/config/symbols.yaml) supplies separate symbol
  lists for ordinary spot capture and liquidation capture.

| Section | How it is assembled | Expected output |
|---|---|---|
| `base` | Import the complete development config | [base/md_capture.yaml](examples/complete_config/genconfig/base/md_capture.yaml) |
| `ci` | Extend `base`; import connection and deployment settings | [ci/md_capture.yaml](examples/complete_config/genconfig/ci/md_capture.yaml) |
| `prod` | Extend `ci`; import `/spot` from the symbol lists | [prod/md_capture.yaml](examples/complete_config/genconfig/prod/md_capture.yaml) |
| `prod-liquidation` | Extend `prod`; restore the old connection, replace the subscription, and adjust logging | [prod-liquidation/md_capture_liquidation.yaml](examples/complete_config/genconfig/prod-liquidation/md_capture_liquidation.yaml) |

For the liquidation exception, `connection` is overridden by importing the
`/connection` fragment of the development config. This restores the old settings
as one mapping. `subscription` is overridden with the liquidation symbol list and
channel, while `log.file` is deleted and `md_log.file` receives a separate capture
path. The other variants retain their own resolved settings.


## Advanced ycfg generation topics

1. Suppress config generation with `ycfg.generate: false`; see the
   [structure rules and example](#structure-of-ycfg-file).
   The section remains available for inheritance, but the flag is not inherited.
   This property is useful if you created a "service" config section - which is not
   supposed to have an associated environment or use case, but it's used by other
   config sections as an extension base.
1. Placeholder substitution during generation
    `{ycfg:gen-var NAME}` substitutes a generation-time variable supplied through the
    process environment or by the build system. Placeholders are allowed in
    application string values and import paths. A placeholder can occupy the whole
    string or appear within text; surrounding text is preserved. `{ycfg:run-var NAME}`
    placeholders pass through generation unchanged for resolution when loading the config.
    Example:
    ```yaml
    log:
      file: '{ycfg:gen-var NFS_PATH}/logs'
    ```
1. Common config libraries
    Config libraries are shared directories of YAML files that app configs can
    import. They avoid duplicating common settings, such as production connection
    parameters used by both a trading strategy and an MD capture app.

    Import paths in `ycfg.import` and `ycfg:import` can start with a location
    placeholder. For example, `NOM_CONFIG_LIB` can point to a well-known config
    library directory in the project. Set it in the process environment, or let
    the build system supply it in the generator's environment:

    ```yaml
    ycfg:
      name: prod
      import: '{ycfg:gen-var NOM_CONFIG_LIB}/connection.yaml'
    symbols: "ycfg:import {ycfg:gen-var NOM_CONFIG_LIB}/markets.yaml:/spot/symbols"
    ```

    Resolve location placeholders before reading the referenced files. An unset
    variable or missing file fails generation. Expanded absolute paths are used
    directly; relative paths are resolved from the ycfg source file's directory.

## Runtime value overrides

When loading a YAML config file, the application follows this order:

1. Parse the application YAML.
2. Resolve runtime environment placeholders.
3. Apply command-line overrides.
4. Perform YAML/C++ mapping and application validation.

For [hard-wired config](app_config_binding.md#hard-wired-config), command-line
overrides target the YAML serialized from the initialized C++ config.

Each stage must succeed before the next begins. An unset required environment
variable causes failure before command-line overrides, even if an override would
replace that value.

There could be multiple reasons for runtime value overrides, but they all boil down to the fact that some parameter is required to have different value at the time when app is executed (versus the time when the app config was defined and generated), for instance.
1. We want to restart the app with different log.level to get more detailed diagnostics.
1. We want to try running the trading strategy with different parameters (e.g. risk aversion).
1. Binance MD API endpoint address changed and we need to restart MD capture with different address.
1. Order gateway requires private key to connect to Order API endpoint and we don't want to provide this key during config generation, so we set the private key only when we start a trading strategy.

All such cases can be done by updating the app config and restarting the app (or dynamically resetting the app config). However this might be inconvenient. For this case we have other ways of runtime value overrides.

### Types of overrides

1. Placeholder substitution at runtime.
    Example `app.yaml`:
    ```yaml
    log:
      level: '{ycfg:run-var LOG_LEVEL}'
      file: '{ycfg:run-var NFS_PATH}/logs'
    ```
    `{ycfg:run-var NAME}` substitutes an environment value within the string when
    loading the config.
    With `NFS_PATH=/srv/data`, the example's `log.file` becomes `/srv/data/logs`.
    An unset variable is an error.
2. Command line config override format
    The app accepts `--set <config-pointer>=<value>` to replace an existing YAML value.
    A missing target is an error. These overrides run after placeholder substitution,
    so command-line values take precedence.
