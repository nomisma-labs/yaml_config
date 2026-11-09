#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary
"""Build libraries under src/, then consume their relocated installed package."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory


def run(*args: str) -> None:
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"{' '.join(args)}\n{result.stdout}\n{result.stderr}")


def write(root: Path, files: dict[str, str]) -> None:
    for relative, text in files.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cmake', 'compiler', 'generator', 'kit'):
        parser.add_argument(f'--{name}', required=True)
    args = parser.parse_args()
    kit = Path(args.kit).resolve()

    with TemporaryDirectory(prefix='nomisma-source-layout-') as temporary:
        root = Path(temporary)
        # Workspace ancestors must not determine a project's library source root.
        source = root / 'services' / 'core' / 'source with spaces'
        main_source = '''#include <values/macros.h>
#include <group/derived_values/macros.h>
#include <custom/macros.h>
#include <flags/macros.h>
import nom.layout_values;
import nom.layout_derived;
import nom.layout_custom;
static_assert(NOM_VALUE == 10 && NOM_DERIVED_VALUE == 20);
static_assert(NOM_CUSTOM_VALUE == 12 && NOM_FLAGS_VALUE == 7);
int main() { return value() == 10 && derived_value() + custom_value() == 42 ? 0 : 1; }
'''
        write(source, {
            'CMakeLists.txt': f'''cmake_minimum_required(VERSION 4.4)
project(source_layout VERSION 1.0.0 LANGUAGES CXX)
include("{kit.as_posix()}/nom_kit.cmake")
nom_project_options()
add_subdirectory(src/values)
add_subdirectory(src/group/derived_values)
add_subdirectory(src/overridden)
# Root-level declarations use the same src/ base as component declarations.
nom_lib(flags HEADERS src/flags/macros.h)
nom_app(check SOURCES main.cpp DEPS nom::values nom::derived_values nom::custom_values nom::flags)
nom_install_package(TARGETS values derived_values custom_values flags)
''',
            'src/values/CMakeLists.txt': '''nom_lib(values
    MODULES values.cppm SOURCES values.cpp HEADERS macros.h)
''',
            'src/values/values.cppm': 'export module nom.layout_values;\nexport int value();\n',
            'src/values/values.cpp': 'module nom.layout_values;\nint value() { return 10; }\n',
            'src/values/macros.h': '#pragma once\n#define NOM_VALUE 10\n',
            'src/group/derived_values/CMakeLists.txt': '''nom_lib(derived_values
    MODULES derived_values.cppm SOURCES derived_values.cpp HEADERS macros.h
    PUBLIC_DEPS nom::values)
''',
            'src/group/derived_values/derived_values.cppm': 'export module nom.layout_derived;\nexport int derived_value();\n',
            'src/group/derived_values/derived_values.cpp': '''module nom.layout_derived;
import nom.layout_values;
int derived_value() { return value() + 20; }
''',
            'src/group/derived_values/macros.h': '#pragma once\n#define NOM_DERIVED_VALUE 20\n',
            'src/overridden/CMakeLists.txt': '''nom_lib(custom_values
    MODULES include/custom/custom_values.cppm SOURCES custom_values.cpp
    HEADERS include/custom/macros.h BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
''',
            'src/overridden/include/custom/custom_values.cppm': 'export module nom.layout_custom;\nexport int custom_value();\n',
            'src/overridden/include/custom/macros.h': '#pragma once\n#define NOM_CUSTOM_VALUE 12\n',
            'src/overridden/custom_values.cpp': 'module nom.layout_custom;\nint custom_value() { return 12; }\n',
            'src/flags/macros.h': '#pragma once\n#define NOM_FLAGS_VALUE 7\n',
            'main.cpp': main_source,
        })
        build, prefix = root / 'build', root / 'prefix'
        run(args.cmake, '-S', str(source), '-B', str(build), '-G', args.generator,
            f'-DCMAKE_CXX_COMPILER={args.compiler}', '-DSOURCE_LAYOUT_BUILD_TESTS=OFF',
            '-DCMAKE_INSTALL_LIBDIR=lib')
        run(args.cmake, '--build', str(build), '--parallel', '2')
        run(str(build / 'bin/check'))
        run(args.cmake, '--install', str(build), '--prefix', str(prefix))
        expected = {
            'values/macros.h', 'values/values.cppm',
            'group/derived_values/macros.h', 'group/derived_values/derived_values.cppm',
            'custom/macros.h', 'custom/custom_values.cppm', 'flags/macros.h',
        }
        installed = {p.relative_to(prefix / 'include').as_posix()
                     for p in (prefix / 'include').rglob('*') if p.is_file()}
        if installed != expected:
            raise RuntimeError(f'Unexpected installed interface paths: {sorted(installed)}')
        relocated = root / 'relocated package'
        prefix.rename(relocated)
        shutil.rmtree(source)
        shutil.rmtree(build)

        consumer = root / 'consumer'
        write(consumer, {
            'CMakeLists.txt': f'''cmake_minimum_required(VERSION 4.4)
project(source_layout_consumer LANGUAGES CXX)
include("{kit.as_posix()}/nom_kit.cmake")
nom_project_options()
find_package(source_layout CONFIG REQUIRED)
nom_app(check SOURCES main.cpp DEPS nom::values nom::derived_values nom::custom_values nom::flags)
''',
            'main.cpp': main_source,
        })
        consumer_build = root / 'consumer-build'
        run(args.cmake, '-S', str(consumer), '-B', str(consumer_build), '-G', args.generator,
            f'-DCMAKE_CXX_COMPILER={args.compiler}', f'-DCMAKE_PREFIX_PATH={relocated}',
            '-DSOURCE_LAYOUT_CONSUMER_BUILD_TESTS=OFF')
        run(args.cmake, '--build', str(consumer_build), '--parallel', '2')
        run(str(consumer_build / 'bin/check'))
        print('PASS: src modules, nested components, explicit BASE_DIR, root declaration and relocated installed consumer')

        for has_library in (True, False):
            repo = root / f'sandbox-{has_library}'
            write(repo, {'CMakeLists.txt': 'project(sandbox_example LANGUAGES CXX)\n'})
            if has_library:
                (repo / 'src/sandbox_example').mkdir(parents=True)
            (repo / 'apps/sandbox').mkdir(parents=True)
            for relative in ('tools/kit/add_sandbox_app.py', 'tools/kit/checks/headers.py'):
                destination = repo / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(kit.parents[1] / relative, destination)
            run(sys.executable, str(repo / 'tools/kit/add_sandbox_app.py'), 'probe')
            generated = (repo / 'apps/sandbox/probe/CMakeLists.txt').read_text()
            if ('DEPS nom::sandbox_example' in generated) != has_library:
                raise RuntimeError(f'Sandbox library dependency mismatch: has_library={has_library}')
        print('PASS: sandbox apps link the src/ main library only when present')


if __name__ == '__main__':
    main()
