# YAML parser research probe

This is an isolated experiment for the [parser recommendation](../../../docs/yaml_config/parser_research.md).
It uses native fkYAML conversions across a named module, plus `import std`.
It does not implement the configuration framework or its mapping macros and is
not part of the normal project build or installed package.

From the repository root, with an fkYAML v0.5.0 checkout available:

```bash
git clone --branch v0.5.0 --depth 1 https://github.com/fktn-k/fkYAML.git /tmp/yaml-config-fkyaml
cmake -S tools/yaml_config/research -B build/parser-research-gcc -G Ninja \
    -DCMAKE_CXX_COMPILER=g++-16 \
    -DFK_YAML_SOURCE_DIR=/tmp/yaml-config-fkyaml \
    -DYAML_PARSER_RESEARCH_INSTALL=OFF
cmake --build build/parser-research-gcc
build/parser-research-gcc/bin/parser_probe_runner
```

Repeat with `build/parser-research-clang` and `-DCMAKE_CXX_COMPILER=clang++-22`.
Both builds/runs passed on 2026-11-09 with fkYAML commit
`5cc0747068b94708aa0986cacd200aa74be4f9e8`. The probe deliberately checks native
behavior, including values that the application profile rejects.
