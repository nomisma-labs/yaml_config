# App/Config binding

The locator chooses a YAML file. The loader parses it, applies runtime
substitution and overrides, then maps and validates the application config.
C++ does not interpret ycfg inheritance, imports, delete, or override instructions.
It parses ordinary YAML using the [shared value profile](ycfg.md#yaml-values-and-portability).
A root `ycfg` key in an ordinary application file is not generation metadata; only
the generator processes structured sources.

## Config resolution

An application has a config lookup key, normally its executable basename, with an
optional explicit key supplied by the application. Automatic file lookup uses
`<key>.yaml`. If `ycfg.gen_file` chooses another filename, select that file
explicitly; the locator does not read ycfg metadata to infer it.

Lookup order:

| Priority | Source | Meaning | Value |
|---|---|---|---|
| 1 | `--config FILE`| Explicit selection of config file at the app start | Config file path |
| 2 | `NOM_CONFIG` env variable | Explicit selection of config set via process env  | Config file path |
| 3 | `<app-directory>/<key>.yaml` beside the executable | Config bundled together with the app |  |
| 4 | `NOM_CONFIG_DIR` env variable | One or more config search paths | Colon-separated list of dirs in order, as described below. |
| 5 | Empty YAML mapping | Last resort: map to C++ using optional or defaulted values | Used only if config lookup finds no file. |

`NOM_CONFIG_DIR` is a colon-separated directory list on the supported Unix
platforms. In each directory, try `<key>.yaml`, then `<key>/<key>.yaml`. Relative
entries are resolved against the executable directory. Relative explicit filenames
from `--config` or `NOM_CONFIG` are resolved against the process working directory.
An empty explicit filename or empty directory-list entry is an error.

Explicit `--config` and `NOM_CONFIG` selections are authoritative: a missing or
unreadable file fails immediately. During directory search, a missing candidate
allows the next candidate; an existing unusable candidate is an error. After a
file is selected, parsing or validation failure does not trigger another search.

For example, with a binary at `build/dev/bin/md_capture`:

```bash
build/dev/bin/md_capture --config build/dev/genconfig/prod/md_capture.yaml
NOM_CONFIG_DIR=../genconfig/prod build/dev/bin/md_capture
NOM_CONFIG_DIR=../config build/dev/bin/md_capture
```

The last command finds the collected default at `config/md_capture/md_capture.yaml`.
The locator does not automatically choose between `dev`, `ci`, and `prod` sections.

The caller can supply the resolved executable path through
`config_options::executable_path`. Otherwise, `current_executable_path()` obtains
it through a library-local OS utility; `argv[0]` alone may be a bare command name.
The private boundary is `src/yaml_config/util/os.h`, with its Linux implementation
selected by CMake. When a common core OS library is available, move that operation
there and keep the config locator calling the same boundary.

When two or more variants of an app (e.g. `mm_strategy` and `mm_strategy_perf`)
use the same config (`mm_strategy.yaml`), they can override `<key>` for automatic
config resolution:

```cmake
nom_app(mm_strategy_perf SOURCES main.cpp COMPILE_OPTIONS -DNOM_CONFIG_KEY=mm_strategy)
```

`<yaml_config/config.h>` supplies `NOM_CONFIG_KEY_VALUE`, which stringifies the
application's `NOM_CONFIG_KEY` definition (or expands to an empty string when it is
not defined). Pass it to `parse_config_arguments()` or set
`config_options::config_key` directly. Explicit `--config` and `NOM_CONFIG`
selections retain their precedence.

### Selecting search directories in Bash

For convenience, the framework provides a set of bash functions that help to manage search config path.

Source [config_path.sh](../../tools/yaml_config/config_path.sh) from
`~/.bash_aliases`, using the absolute path to your checkout:

```bash
source /path/to/yaml_config/tools/yaml_config/config_path.sh
```

If `~/.bashrc` already loads `~/.bash_aliases`, new Bash sessions will have the
commands available. Run `source ~/.bash_aliases` to load them in an existing shell.
Sourcing the script defines the commands without changing the current selection.

| Command | Effect |
|---|---|
| `cfg_set` | Replace `NOM_CONFIG_DIR` with the current directory. |
| `cfg_add` | Append the current directory unless it is already listed; start a new list if unset or empty. |
| `cfg_clear` | Unset `NOM_CONFIG_DIR` and print the cleared state. |
| `cfg_print` | Print one search directory per line, in order; report if unset or empty. |

Both `cfg_set` and `cfg_add` print the resulting search path after a successful
call, using the same format as `cfg_print`, including when a directory is already listed.

```bash
cd /srv/config/prod
cfg_set
# /srv/config/prod
cd /srv/config/shared
cfg_add
# /srv/config/prod
# /srv/config/shared
cfg_print
# /srv/config/prod
# /srv/config/shared
cfg_clear
# NOM_CONFIG_DIR is not set
```

All commands take no arguments. The setters export absolute physical paths
(resolving directory symlinks), so later `cd` commands do not change the selection.
Repeated additions preserve the existing order. Spaces in paths are supported;
colons are rejected because they separate entries. `cfg_add` rejects an existing
list with empty entries; `cfg_set` can replace it.
`cfg_clear` unsets the variable because an empty directory list is invalid for the locator.

The selection applies to this shell and programs subsequently launched from it.
Other existing terminals are unaffected. Explicit `--config`, `NOM_CONFIG`, and
config files beside the executable still take precedence over this directory list.

## Loading and overrides

After config resolution, the loader:

1. Reads the selected file and parses its YAML mapping, or initializes the hard-wired
   config if lookup found no file (see [hard-wired config](#hard-wired-config)).
2. Substitutes runtime environment placeholders in string values.
3. Applies command-line overrides in order, checking candidate mappings.
4. Maps the final mapping to the application's [C++ config types](yaml_cpp_mapping.md).
5. Calls the root config's `validate()` protocol. On success, the config is ready
   for application initialization.

Config structures support `validate()` returning `std::expected<void, std::string>`:
success has no value; failure carries an error message. See [validation](yaml_cpp_mapping.md#validation-boundaries).

A read, parse, mapping, or validation error stops config initialization.

### Runtime overrides

Runtime overrides modify the YAML before final C++ mapping and validation.
For hard-wired config, the override targets come from serializing the initialized
C++ config, as described [below](#hard-wired-config).
First substitute `{ycfg:run-var NAME}` placeholders, then apply
`--set <config-pointer>=<value>` arguments in command-line order, as described in
[runtime value overrides](ycfg.md#runtime-value-overrides).

Every stage must succeed before the next begins. A missing environment variable
fails before CLI overrides, even if an override would replace that property.
Missing override targets are errors. Repeated overrides of the same target apply
in order, with the last successful assignment taking precedence.

Overrides preserve the same string-first policy as the rest of the configuration
workflow. Split each `POINTER=value` at its first `=`. Try the entire value text as
a string at that existing target and probe mapping of the complete candidate. If
that fails, parse the text as one complete YAML document containing the replacement
value, validate the shared value profile, replace the target, and probe again.
The document may hold a scalar, sequence or mapping. Empty streams, null, a second
document, and unsupported YAML features fail this attempt. The result replaces
only the addressed value; it never becomes a document patch.
No placeholders are expanded in CLI values.

For example, `/log/level=debug` supplies a string, `/connection/timeout_s=20`
supplies an integer when the destination is numeric, and
`'/subscription/symbols=["btcusdt", "ethusdt"]'` supplies an array. A string member
accepts the literal text `20` or `true` as a string. Flow mappings use YAML syntax,
for example `'/connection={address: localhost, timeout_s: 20}'`. The text `null`
can be accepted by the first, string attempt; a parsed YAML null is rejected.
Quote the whole shell argument when it contains spaces, braces or brackets.

Pointers follow the [same token rules as imports](ycfg.md#import-a-fragment-of-a-yaml-file).
The empty pointer may replace the whole document, which must still map to a root
mapping. Missing targets, array append syntax, and key creation are unsupported.

Each override must map successfully before the next is attempted; a later override
cannot repair another malformed field that prevents the current mapping probe.
Each probe maps a private freshly initialized C++ candidate, and failed attempts
leave the document unchanged. Application validation runs only after all overrides
have been applied.

```bash
build/dev/bin/md_capture --config build/dev/genconfig/prod/md_capture.yaml \
    --set /connection/timeout_s=20 --set /log/level=debug
```

## Application integration

Keep location, YAML loading, and [typed mapping](yaml_cpp_mapping.md) available
separately. The core API accepts parsed options; an optional command-line adapter
handles `--config` (`-c`), repeated `--set`, `--check-config` (`-k`), and `--help`
(`-h`) alongside app-owned options. Other arguments remain available to the app.

Return the selected path and selection source with the load result, so the app can
report which config it used. For hard-wired config, report that source without a
file path. Failures identify the loading stage and relevant
file or config pointer. The library reports errors to its caller; the application's
entry point chooses logging and exit behavior. No logger or assembly dependency is
required to load config.

`--check-config` runs the same loading and validation steps, then exits before
starting application services. The application uses `print_config_check()` for
successful output and handles loading errors through its usual error path.

Import `nom.yaml_config` and link `nom::yaml_config`. The main entry point is
`load_config<Config>(options)`, returning `loaded_config<Config>` with `.value`
and `.location` (`.path`, `.source`, and `.search_details`). The root type must provide
`validate() const` returning `std::expected<void, std::string>`.

```cpp
const auto arguments = nom::ycfg::parse_config_arguments(argc, argv, NOM_CONFIG_KEY_VALUE);
if (arguments.help)
{
    nom::ycfg::print_config_help(std::cout);
    return 0;
}
// Parse or reject remaining application arguments before config I/O.
const auto loaded = nom::ycfg::load_config<app_config>(arguments.options);
if (arguments.options.check_config)
{
    nom::ycfg::print_config_check(std::cout, loaded, arguments.options.check_mode);
    return 0;
}
// Initialize application services with loaded.value.
```

The adapter returns app-owned arguments in `.remaining_arguments`, preserves `--`
and everything following it, and accepts `--config` once. It uses
Boost.Program_options with exact long option names; `--config=FILE`, `-cFILE`, and
`--set=POINTER=value` are also accepted. `--help` and `-h` set `.help` rather than
remaining in the application arguments. `print_config_help()` uses the same
Boost option descriptions as the parser, so names, aliases, and usage stay together.
See the [complete application example](../../apps/config_workflow_example/README.md).

The [application-argument example](../../apps/config_args_example/README.md) passes
`.remaining_arguments` and `.help` to its own Boost.Program_options parser before
calling `load_config()`. It prints application help alongside `print_config_help()`.
Handle application help and argument validation at this point,
so help does not require a readable config and invalid app arguments fail before
config loading. Both parsers and the application startup remain controlled by
`main()`.

| Function | Purpose |
|---|---|
| `locate_config(options)` | Choose a file or the hard-wired source. |
| `read_config(location)` | Read a YAML mapping, or return an empty mapping for the hard-wired source. |
| `substitute_runtime_variables(document)` | Expand runtime placeholders in YAML values. |
| `apply_config_overrides(document, overrides, probe)` | Apply ordered overrides using a caller-supplied mapping check. |
| `load_config<Config>(options)` | Run the full sequence, fkYAML mapping, and root validation. |
| `print_config_help(output)` | Print the framework's option descriptions, aliases, and usage. |
| `print_config_check(output, loaded, mode)` | Print a successful check in `full`, `view`, or `silent` mode. |

Location, substitution, and typed loading accept an optional `environment_lookup`
callback; by default they read the process environment. Mapping checks use native
fkYAML conversion, for example `document.get_value<Config>()`, independently of the
locator. `config_error` reports `.stage`, `.file`, `.pointer`, and an explanatory
`what()` message. Mapping diagnostics retain config pointers. Native parse errors
include line/column information when available; synthesized values have no
original source span.

### Check output

Choose a mode with `--check-config=MODE`, `--check-config MODE`, `-k MODE`, or
`-kMODE`. A bare `--check-config` or `-k` selects `full`. Repeated check options
use the last mode.

| Mode | Successful standard output |
|---|---|
| `full` | Lookup details, selected source/path (or hard-wired defaults), then the final YAML. |
| `view` | Only the final YAML, suitable for redirecting to a file. |
| `silent` | Only `OK`. |

In `full` mode, the locator records its actual lookup in `.location.search_details`:
selection inputs, executable and config key when used, missing candidates, and the
selected file or fallback. Lookup stops at the first selected file; lower-priority
locations are reported as not searched. No second lookup is performed for reporting.
Normal loading and the other check modes leave this string empty.

The final YAML is serialized from the validated C++ config, after runtime variable
substitution and ordered command-line overrides. It includes C++ defaults and
omits absent optional members; input fields outside the C++ mapping
are omitted. Checking does not modify the source file. Failures produce no success
report; the examples print diagnostics to standard error and return status 1.

```bash
build/dev/bin/config_workflow_example -c config.yaml -k
build/dev/bin/config_workflow_example -c config.yaml --check-config=view
build/dev/bin/config_workflow_example -c config.yaml -k silent
```

A separate word is consumed as the mode only when it is `full`, `view`, or `silent`;
other words remain application arguments. Use `--` before an application positional
argument with one of those names, such as `-k -- view`. An unknown attached mode,
such as `--check-config=brief`, is an argument error.

## Hard-wired config

If config resolution exhausts its lookup steps without finding a file, the loader
provides an empty YAML mapping as a last resort and tries to map it to C++.
Successful mapping means the config can be initialized entirely from optional or
defaulted values; this is a **hard-wired config**. Missing required values cause
mapping to fail. The resulting C++ config must still pass `validate()`.

Runtime command-line overrides can still be applied to hard-wired config. After
successful mapping of an empty mapping, serialize the initialized C++ config to YAML and apply
the overrides to that object's values. Map the updated YAML back to C++, then
validate the final config. Empty optional members are omitted by serialization,
so they have no existing pointer target for `--set`. An engaged optional has a
target. Omission retains a C++ initializer; neither a delete directive nor a null
sentinel is interpreted by the runtime loader.

This fallback applies only when lookup finds no file without an error. It does not
rescue a broken explicit path, an unreadable or malformed file, or failed mapping
or validation.
