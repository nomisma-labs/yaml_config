# yaml_config

<!-- repo-description:begin -->
`yaml_config` is Nomisma's YAML application configuration framework under design.
Its `ycfg` language, Python tools, CMake integration, loader and C++ mappings follow
the workflow of `json_config` and `toml_config`. A project chooses one framework.

**Documentation review stage:** the template scaffold and parser research are
complete. The guides specify the proposed behavior; the library, generator and
example applications are not implemented yet. Start with the
[guide index](docs/yaml_config/README.md), [parser research](docs/yaml_config/parser_research.md)
and [work plan](TODO.md).
<!-- repo-description:end -->

## Build

At this stage these commands validate the empty scaffold. The isolated parser
probe has [separate instructions](tools/yaml_config/research/README.md). Framework
build/run commands in the guides become available after implementation.

Install the tools listed in [docs/kit/build.md](docs/kit/build.md#toolchain), then:

```bash
./bootstrap.sh                   # fresh builds, tests and checks for every preset but release
cmake --workflow --preset dev    # configure, build and test one preset
```

Everyday commands, the presets and what bootstrap checks: [docs/kit/build.md](docs/kit/build.md).

## Docs

Agents start at [AGENTS.md](AGENTS.md), people at [docs/](docs/README.md). Adding a library, an
app, tests or a dependency: [docs/kit/howto.md](docs/kit/howto.md). Releases: [CHANGELOG.md](CHANGELOG.md).
