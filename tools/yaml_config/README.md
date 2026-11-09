# yaml_config tools

> **Design for review — implementation pending.** APIs, commands and tests below
> specify the intended behavior. See [the implementation plan](../../TODO.md) for status.

Shared build and maintenance tools live in [tools/kit](../kit/README.md).

| File | Purpose | Documentation |
|---|---|---|
| [config_path.sh](config_path.sh) | Sourceable Bash commands for selecting config search directories. | [Bash setup and commands](../../docs/yaml_config/app_config_binding.md#selecting-search-directories-in-bash) |
| `ycfggen.py` (planned) | Command-line YAML config generator and metadata listing. | [Config generation](../../docs/yaml_config/config_generation.md) |
| `ycfg.py` (planned) | Python engine that resolves ycfg sections into application YAML in memory. | [Python API](../../docs/yaml_config/config_generation.md#python-api) |
| `ycfg_build.py` (planned) | Utility called by CMake to manage generation plans, output ownership, and successful build records. | [Build integration](../../docs/yaml_config/build_system_integration.md) |

`yaml_codec.py` supplies the shared Python YAML value profile and deterministic writer.

The Bash helper is already copied unchanged from the other frameworks. The
[research probe](research/README.md) is available now. Generator commands below
become available after implementation and installation of the planned
`requirements.txt` dependency set.

Run the generator from the repository root:

```bash
python3 tools/yaml_config/ycfggen.py --help
```
