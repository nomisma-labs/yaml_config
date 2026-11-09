# yaml_config tools

> **Implementation in progress.** The Python codec and generator are available;
> C++ parity and the remaining framework stages are tracked in [TODO.md](../../TODO.md).

Shared build and maintenance tools live in [tools/kit](../kit/README.md).

| File | Purpose | Documentation |
|---|---|---|
| [config_path.sh](config_path.sh) | Sourceable Bash commands for selecting config search directories. | [Bash setup and commands](../../docs/yaml_config/app_config_binding.md#selecting-search-directories-in-bash) |
| [ycfggen.py](ycfggen.py) | Command-line YAML config generator and metadata listing. | [Config generation](../../docs/yaml_config/config_generation.md) |
| [ycfg.py](ycfg.py) | Python engine that resolves ycfg sections into application YAML in memory. | [Python API](../../docs/yaml_config/config_generation.md#python-api) |
| [ycfg_build.py](ycfg_build.py) | Utility called by CMake to manage generation plans, output ownership, and successful build records. | [Build integration](../../docs/yaml_config/build_system_integration.md) |

`yaml_codec.py` supplies the shared Python YAML value profile and deterministic writer.

The Bash helper is already copied unchanged from the other frameworks. The
[research probe](research/README.md) is available now. Install the generator dependency with
`python3 -m pip install -r tools/yaml_config/requirements.txt` in your Python
environment before running generator commands.

Run the generator from the repository root:

```bash
python3 tools/yaml_config/ycfggen.py --help
```
