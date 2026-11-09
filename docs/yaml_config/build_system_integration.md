# Build system integration

> **Build integration implemented.** The CMake helpers and build utility are available.
> The documented C++ applications are implemented; acceptance checks are tracked in [TODO.md](../../TODO.md).

The shared CMake kit handles config collection. `yaml_config` defines generation,
config targets, test registration, and installation of generated configs.

## Collection

An application declared with `nom_app()` automatically collects its adjacent
`config/` directory. For example, in `apps/md/md_capture/CMakeLists.txt`:

```cmake
nom_app(md_capture SOURCES main.cpp BUILD_FOR dev ci)
```

The [kit collector](../../cmake/kit/api/targets.cmake) and config generator use these locations:

| Source or artifact | Build location |
|---|---|
| Executable | `build/<preset>/bin/md_capture` |
| `apps/md/md_capture/config/` | `build/<preset>/config/md_capture/` |
| Generated `prod` config | `build/<preset>/genconfig/prod/md_capture.yaml` |

`../../cmake/yaml_config/config.cmake` defines absolute build paths:

| Constant | Path |
|---|---|
| `NOM_COLLECTION_CONFIG_DIR` | `${CMAKE_BINARY_DIR}/config` |
| `NOM_GEN_CONFIG_DIR` | `${CMAKE_BINARY_DIR}/genconfig` |
| `NOM_CONFIG_BUILD_DIR` | `${CMAKE_BINARY_DIR}/CMakeFiles/yaml_config` (plan and records) |

The helpers use these constants for generation, tests, and cleanup. Installation
preserves the config layout relative to the install prefix. Making collection
configurable will also require updating the shared kit's collector.

Collection refreshes the destination whenever the app builds, including removing
files deleted from the source directory. Executables declared in the same source
directory share one collection target. App directory basenames must be unique in
the combined build because they share the collection root.

Collection copies files without interpreting them. It remains useful for an app
that loads an ordinary YAML config directly and never runs the generator.

Donor files outside the app folder need explicit locations or collection into a
matching directory layout. Copying only the app folder does not preserve imports
such as `../shared/settings.yaml`. For shared config libraries, the build can supply
`NOM_CONFIG_LIB` in the generator's environment, pointing to an absolute project
directory. Imports reference it as `{ycfg:gen-var NOM_CONFIG_LIB}`; see
[common config libraries](ycfg.md#advanced-ycfg-generation-topics).

## Registering generation

The generator needs Python 3.11+ and the pinned PyYAML requirements.
Use an explicit `Python_EXECUTABLE` when configuring CMake to select a virtual
environment. Configuration must report a missing dependency before planning
outputs; see [generator setup](config_generation.md).

The build preset selects applications and compiler settings; a ycfg section
selects application configuration. For example, a `dev` build may generate `prod`
configs. These selections must not be implicitly tied together.

Include `../../cmake/yaml_config/config.cmake` once after the shared
kit. It is included by `yaml_config` itself; a client can also include it directly
from the `yaml_config` checkout. Register generation after declaring the app:

```cmake
include("${yaml_config_SOURCE_DIR}/cmake/yaml_config/config.cmake")

nom_app(md_capture SOURCES main.cpp BUILD_FOR dev ci)
nom_ycfg_generate(md_capture
    SOURCE md_capture_ycfg.yaml
    ENVIRONMENT "NOM_CONFIG_LIB=${PROJECT_SOURCE_DIR}/config_lib")
```

`SOURCE` is required. Omit `SECTIONS` to generate all sections with
`ycfg.generate=true` (the default). To select a subset, use:

```cmake
nom_ycfg_generate(md_capture SOURCE md_capture_ycfg.yaml SECTIONS ci prod)
```

Explicitly selecting a section with `ycfg.generate=false` is an error.
`nom_ycfg_generate()` returns immediately when `nom_build_apps` is OFF, before
looking up the app or validating generation arguments. Generation is independent
of `nom_build_tests` and `nom_install`; installation rules for generated files
still require `nom_install`.
`ENVIRONMENT` supplies optional `NAME=value` assignments on top of the build
process environment. The helper registers:

| Setting | Example |
|---|---|
| App name | `md_capture`, already declared by `nom_app()` |
| Source relative to its config directory | `md_capture_ycfg.yaml` |
| Sections to generate and install | All sections with generation enabled, or an explicit `SECTIONS` list |
| Generator command | Discovered Python interpreter with PyYAML installed, plus `ycfggen.py` |

By default, building the app only collects its configs. This supports development
with an ordinary default YAML config and does not require generation-time donors
or variables. Run `ninja config` to generate all registered configs, or build
`<project>_generate_ycfg_<app>` to collect and generate one app's configs without
compiling it, for example `yaml_config_generate_ycfg_md_capture` in a project
named `yaml_config`. Add the optional `BUILD_WITH_APP` flag to `nom_ycfg_generate()`
when building that app should also generate its configs. This choice is explicit
and independent of the preset.

The helper resolves section selection and output filenames through the generator's
[listing operation](config_generation.md#command-line), so the app's CMake file
does not repeat `ycfg.gen_file`. Changing the ycfg source causes CMake to refresh
the output plan on the next build. With `SECTIONS` omitted, newly added sections
with generation enabled are included automatically. If none are enabled,
generation produces no files and removes previously owned outputs.

Config-generation CMake scripts and functions belong in `yaml_config`, including
registration, build dependencies, and installation of generated outputs.
`cmake_template` provides the existing config collector and remains unaware of
generation. Client repositories use the helpers supplied by `yaml_config` and
declare their app settings through `nom_ycfg_generate()`.
The app name is the exact target name supplied to `nom_app()` in the same project.
Config-generation targets and CTest names retain their project prefix.
One app registers one source; apps sharing that source must use the same section
selection and explicit environment. Source, section and output names containing
semicolons or newlines are unsupported by the CMake integration.

## Build sequence

1. Collect the app's source configs.
2. Run the generator against the collected source, with explicit input/output paths.
3. Check that every requested section produced its expected YAML mapping.
4. Record the successful output set.

The generation target depends on collection. An app depends on generation only
with `BUILD_WITH_APP`. Several executables sharing the same source and selection
share one generation operation; each app can choose whether to build it automatically.
Conflicting registrations or overlapping output ownership are errors.

Generation runs each time its target is requested, observing donor-file changes,
removed files, and generation-time environment changes. Unchanged output contents
need not be rewritten, and config-only changes do not require recompiling C++.
CMake discovers the Python interpreter and passes absolute tool paths so execution
does not depend on the working directory.

Maintain a build record of the generated files and their content hashes. Invalidate
it before a new attempt, and mark it successful only after generation completes.
Remove renamed or deselected files owned by that registration without touching
other apps' outputs. Valid runtime `{ycfg:run-var NAME}` placeholders are allowed in
generated files; C++ type and application validation happen when the app loads them.

The build utility `../../tools/yaml_config/ycfg_build.py` keeps ownership and
success records under `CMakeFiles/yaml_config/records/`. Success is invalidated
before collection, including an app-only collection or a failed collection. The
configure-time output plan lives separately so cleaning does not require a CMake
reconfigure.

## Repository-wide config target

The `config` target runs all config collection and generation tasks
registered in the current build tree. It includes apps that only collect ordinary
YAML files and apps whose executables are excluded from the normal build by
`BUILD_FOR`. Generation uses the sections selected in each app's registration.

```bash
ninja -C build/dev config
```

The `yaml_config` CMake helpers provide this aggregate target. It depends directly
on the collection and generation targets, so it can prepare configs without
compiling applications. Generation follows its input collection, shared operations
run once, and any collection or generation error fails the target.

## Testing generated configs

`yaml_config` defines an opt-in CMake helper,
`nom_ycfg_test(app_name config_section)`, for registering config checks with CTest:

```cmake
nom_ycfg_test(md_capture prod)
```

Each call registers one test that runs the app with the section's generated config
using `--config FILE --check-config`. It exercises config loading, YAML/C++ mapping,
and validation, then exits before starting application services; see
[app integration](app_config_binding.md#application-integration).
Build the app and generate its config before running CTest, for example with
`ninja all config`. Tests are registered
only for explicitly requested app/section pairs, allowing projects to control
the size of their test suite.
Call `nom_ycfg_test()` after `nom_ycfg_generate()`. The helper registers nothing
unless both `nom_build_tests` and `nom_build_apps` are ON, so helper calls need no
outer condition. The section must be included in that app's generation selection.
Test names are `<project>_<app>_config_<section>` and carry the `config` label.

See [config_workflow_example](../../apps/config_workflow_example/README.md) for a complete app using
`nom_ycfg_generate()` and `nom_ycfg_test()` with `dev` and `prod` sections.
Its CLI tests check startup and failure behavior; the
[consumer tests](../../tests/integration/library_usage/README.md) generate and load
configs using both source and installed C++ libraries.

## Cleaning build configs

The `clean_config` target removes all collected and generated configs
for the current build tree, including current outputs and files left by renamed
or removed apps or sections. It deletes both `build/<preset>/config/` and
`build/<preset>/genconfig/`, together with generation success records, without
needing to inspect the current ycfg definitions. Original source configs remain
intact. Run `ninja config` to collect and generate all registered configs again.
Building an app restores its collected configs; generation also runs if that app
uses `BUILD_WITH_APP`.

```bash
ninja -C build/dev clean_config
```

The same cleanup is included in `ninja clean`. The `yaml_config` CMake helpers
register the cleanup target and paths independently of individual app registrations,
so cleanup remains available even after the last generation registration is removed.
Normal clean integration uses CMake's
[ADDITIONAL_CLEAN_FILES](https://cmake.org/cmake/help/latest/prop_dir/ADDITIONAL_CLEAN_FILES.html)
directory property.

## Installation and deployment

When installation is enabled, the current kit installs selected executables under
`<prefix>/bin/` and their collected sources under `<prefix>/config/<app-directory>/`.
The generation integration additionally installs its selected outputs under
`<prefix>/genconfig/<section>/<gen_file>`.

Build apps and generate configs before installing:

```bash
ninja -C build/dev all config
cmake --install build/dev --prefix /path/to/staging
```

If building them separately, run `ninja config` after the last app build: app
collection invalidates prior generation records. Installation must reject a missing success record,
missing output, or output changed since successful generation. An app excluded by
`BUILD_FOR` remains explicitly buildable but is not installed. A shared config
group is installed when one of its apps is selected.

Generated YAML is runtime data; running the application does not require Python
or access to donor files. Deployment can select a generated file explicitly with
`--config`, or expose a section directory through `NOM_CONFIG_DIR`, as described
in [app/config binding](app_config_binding.md). Installing into an existing prefix
does not remove files from older installations; use a fresh staging prefix when
checking the intended deployment contents.
