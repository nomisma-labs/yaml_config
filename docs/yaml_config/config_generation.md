# Config generation

> **Design for review — implementation pending.** APIs, commands and tests below
> specify the intended behavior. See [the implementation plan](../../TODO.md) for status.

`ycfggen.py` produces application YAML from ycfg sources. It can run directly or
through the [build system](build_system_integration.md), using Python 3.11+ and
PyYAML 6.0.3. The repository-owned `yaml_codec.py` resolves values using the
[shared profile](ycfg.md#yaml-values-and-portability), rather than PyYAML's default
YAML 1.1 implicit typing. It emits deterministic YAML with type-preserving quotes.
Implementation will provide `tools/yaml_config/requirements.txt`; install it in
the Python environment used by the generator and CMake. The parser choice and
Python/C++ consistency requirements are in [parser research](parser_research.md).
The [ycfg definition](ycfg.md) owns the language rules.

## Inputs and outputs

The tool accepts either one source file or an input directory tree, such as the
collected config directory:

```text
config/<app>/<app>_ycfg.yaml          source definition
config/<app>/*.yaml                  app-owned donor files
genconfig/<section>/<gen_file>       generated application config
```

Discovery scans recursively for `*_ycfg.yaml`, including files directly in the
input directory. The app name comes from the filename without `_ycfg.yaml`;
parent directory names do not affect discovery.

The output filename follows `ycfg.gen_file`, its inheritance rules, and the
source-derived default. Section names are exact identifiers; they need not be
deployment environments. The generator does not invent a `base` section.

Only sections with `ycfg.generate = true` produce files; omission means `true`.
Suppressed sections can still supply inherited values and filenames. Selecting
a suppressed section explicitly for generation is an error. Selecting a child
resolves its ancestors without automatically writing their outputs.

Two results claiming the same output path are an error. Output section names and
filenames must stay within the output directory; absolute output paths and path
traversal are rejected. Input and output locations must be separate.
With `--input-dir`, the input and output directory trees must not overlap. With
`--source`, the output may be a build subdirectory beside the source, but neither
the source nor any imported donor may reside inside the output tree. These checks
also follow symbolic links. A destination that is already a directory, or conflicts
with another output's file/directory path, is an error.

## Command line

| Argument | Meaning |
|---|---|
| `--source FILE` | Read one ycfg file directly. |
| `--input-dir DIR` | Recursively discover `*_ycfg.yaml` files under `DIR`. Mutually exclusive with `--source`. |
| `--app NAME` | Select exactly one `NAME_ycfg.yaml` anywhere under `--input-dir`; otherwise process all discovered sources. |
| `--config NAME` | Select a section; repeat to select several. Requires `--source` or `--app`. Omission selects all generating sections. |
| `--output-dir DIR` | Required output root when generating files. |
| `--list` | Report app, section, resolved output filename, and `generate` flag as JSON without generating files. No output directory is required. |
| `--verbose` | Include source and output paths in diagnostics. |

Exactly one input mode is required. An unknown app or section, an ambiguous
`--app` match, or discovery that finds no ycfg files is an error. App selection
happens before reading source contents, so unrelated apps are not evaluated.
A source whose sections are all suppressed is valid and produces no files when
no explicit section selection is requested.

`--list` prints a JSON array with `app`, `section`, `gen_file`, and `generate`
fields. Discovered sources are sorted by app name, then path; sections retain
source order. In `--source` mode, the app name is the source basename without `_ycfg.yaml` (or the
filename stem for another suffix). Listing a suppressed section explicitly is
allowed. This JSON is tool metadata only; source definitions and generated
application configs are YAML. It keeps the listing/build interface simple and
independent of YAML application values. Verbose messages and errors go to stderr, keeping stdout usable as JSON.

For one application:

```bash
python3 tools/yaml_config/ycfggen.py --input-dir build/dev/config --app md_capture \
    --config prod --output-dir build/dev/genconfig
```

To generate directly from the [complete example](../../apps/md_capture/config/md_capture_ycfg.yaml):

```bash
python3 tools/yaml_config/ycfggen.py \
    --source apps/md_capture/config/md_capture_ycfg.yaml \
    --config prod --output-dir build/dev/genconfig
```

Both examples select `prod/md_capture.yaml`. Omitting `--config` in the second
command also generates the other enabled sections, including
`prod-liquidation/md_capture_liquidation.yaml`. To process all collected apps,
omit both `--app` and `--config` from the first command.

## Resolution and errors

The generator checks metadata and inheritance for every section, then resolves
imports and instructions only for selected sections and their ancestors using
the merge rules in [ycfg.md](ycfg.md). Unrelated sections do not require their
donor files or generation variables to be available. `ycfg.name`, `ycfg.extend`,
and `ycfg.gen_file` are literal strings; they do not support placeholders.
Relative imports are resolved from the source file's directory. Imported ordinary
YAML stays literal during generation.
Fragment references use `file.yaml:/pointer`; see the
[fragment import rules](ycfg.md#import-a-fragment-of-a-yaml-file).

`{ycfg:gen-var NAME}` is resolved during generation; an unset variable is an error.
`{ycfg:run-var NAME}` remains in the output for substitution when loading the config.
The generator does not require the runtime environment or know the application's C++ types.
Import paths can use generation-time variables such as `{ycfg:gen-var NOM_CONFIG_LIB}`,
provided through the process environment or supplied by the build system. Expand
these placeholders before resolving and reading the files; the tool must not guess
library locations from the caller's working directory.

Missing files or fragments, malformed YAML, invalid metadata, unknown instructions
in ycfg-authored content, and failed delete/override operations stop generation.
Diagnostics identify the source file, section, and property or import involved.
Return zero on success, nonzero on failure; do not substitute empty objects for
failed imports or publish a partially resolved config. Validate values against the
[shared YAML profile](ycfg.md#yaml-values-and-portability), including scalar categories, numeric bounds, null rejection and duplicate-key
checks that Python's usual constructors do not enforce consistently with C++.

Resolve and check all selected results before publishing them. Use temporary files
and replace completed outputs; leave identical output files unchanged. The build
must record success only after every selected output is published. A failed run
must not make older files appear to be current successful results.

Each file replacement is atomic; the complete output set is not a filesystem
transaction. A publication failure returns nonzero and removes temporary files,
but completed replacements may remain. The [build integration](build_system_integration.md#build-sequence)
owns success records and removal of stale outputs; the standalone CLI leaves
unselected or previously generated files in place.

`--list` parses the source using the shared YAML profile, validates metadata and
resolves output names. It does not resolve application instructions, imports or
environment placeholders, and does not perform C++ mapping. Without a section filter it reports all
sections, including suppressed ones. It supports build planning and deployment
selection; it is not application validation. The [C++ loader](app_config_binding.md)
performs runtime substitution, overrides, mapping, and validation.

## Python API

The `../../tools/yaml_config/ycfg.py` resolves configs in memory
without writing output files. From the repository root, use it directly:

```python
from tools.yaml_config.ycfg import generate_configs

configs = generate_configs(
    "apps/md_capture/config/md_capture_ycfg.yaml",
    sections=["prod"],
)
print(configs["prod"].gen_file)  # md_capture.yaml
print(configs["prod"].config)    # resolved application YAML as a Python dict
```

YAML mappings become dictionaries, sequences become lists, and scalar values use
`str`, `bool`, `int` and finite `float`. Dates stay strings. Null has no supported
representation. An empty mapping is `{}` in Python and serializes as `{}\n`; an
empty file is not an empty mapping. The writer's output must parse back to the
same values and scalar categories in both Python and C++. Comments, quoting style
and source key order are not preserved. Map keys are emitted in deterministic
lexical order; sequences retain their order.

`generate_configs()` returns a dictionary keyed by section name, preserving source
order, and writes no files.
Omit `sections` to resolve all sections with generation enabled. Generation
variables use the process environment by default. An explicit `environment`
mapping replaces it for that call. Invalid configuration raises `config_error`
with source context.

`generate_configs_with_inputs()` accepts the same arguments and returns
`(configs, input_paths)`. The paths are a set of absolute `Path` objects with
symbolic links resolved, covering the source and donor files read for selected
sections and their ancestors. The CLI uses them to protect inputs from output writes.

## Tests

Run the engine and CLI tests directly from the repository root, or through the
normal CTest workflow:

```bash
python3 -m unittest discover -s tests/unit/ycfg -p '*_test.py' -v
python3 -m unittest discover -s tests/integration/ycfggen -p '*_test.py' -v
```
