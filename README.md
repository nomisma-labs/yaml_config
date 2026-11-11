# yaml_config

<!-- repo-description:begin -->
`yaml_config` provides Nomisma's YAML application configuration framework: the
`ycfg` language, Python generator, CMake generation and deployment helpers,
C++ loader, and native fkYAML mappings. Its tools and conventions follow
`json_config` and `toml_config`. A project chooses one framework.

Start with the [guide index](docs/yaml_config/README.md),
[parser research](docs/yaml_config/parser_research.md) and [work plan](TODO.md).
<!-- repo-description:end -->

## Build

Install the tools listed in [docs/kit/build.md](docs/kit/build.md#toolchain), then:

```bash
git submodule update --init --recursive
python3 -m pip install -r tools/yaml_config/requirements.txt
./bootstrap.sh                   # fresh builds, tests and checks for every preset but release
cmake --workflow --preset dev    # configure, build and test one preset
```

Everyday commands, the presets and what bootstrap checks: [docs/kit/build.md](docs/kit/build.md).

## Docs

Agents start at [AGENTS.md](AGENTS.md), people at [docs/](docs/README.md). Adding a library, an
app, tests or a dependency: [docs/kit/howto.md](docs/kit/howto.md). Releases: [CHANGELOG.md](CHANGELOG.md).

## License

This project is licensed under [MIT](LICENSE). Copyright belongs to Alexey Tkachenko.
The synchronized build kit retains its [MIT notice](cmake/kit/LICENSE); third-party
dependencies retain their own licenses.
