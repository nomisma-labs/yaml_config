# yaml_config

<!-- repo-description:begin -->
`yaml_config` is the YAML application configuration framework under design for Nomisma.
It follows the user workflow and C++ mapping conventions of `json_config` and `toml_config`.
The repository currently contains the template scaffold; parser research and documentation
come first, with implementation starting after design review.
<!-- repo-description:end -->

## Build

Install the tools listed in [docs/kit/build.md](docs/kit/build.md#toolchain), then:

```bash
./bootstrap.sh                   # fresh builds, tests and checks for every preset but release
cmake --workflow --preset dev    # configure, build and test one preset
```

Everyday commands, the presets and what bootstrap checks: [docs/kit/build.md](docs/kit/build.md).

## Docs

Agents start at [AGENTS.md](AGENTS.md), people at [docs/](docs/README.md). Adding a library, an
app, tests or a dependency: [docs/kit/howto.md](docs/kit/howto.md). Releases: [CHANGELOG.md](CHANGELOG.md).
