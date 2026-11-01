# wuff

`wuff` is the analysis engine behind the [WooWoo Language Server](https://github.com/woowoo-dev/woowoo-language-server).
It is written in C++, parses WooWoo with tree-sitter and is distributed as a Python package (pybind11).

## Installation

```bash
pip install wuff
```

[PyPI](https://pypi.org/project/wuff/#files) has wheels for CPython 3.10 to 3.15 on Linux, macOS and Windows.
Elsewhere pip builds `wuff` from source, which needs a C++20 compiler.

## Development

```bash
git clone https://github.com/woowoo-dev/wuff.git
cd wuff
pip install ".[test]"
pytest
```

CMake downloads the C++ dependencies (tree-sitter, yaml-cpp, pybind11) during the build, so it needs network access.

## Releasing

1. Bump `version` in `setup.py` and add a section to `CHANGELOG.md`.
2. Create a GitHub release with the tag `vX.Y.Z`.
3. The *Wheels* workflow builds the wheels and the sdist. After an approval in the `pypi` environment it uploads them to PyPI (Trusted Publishing, no token).

## Changelog

See [CHANGELOG.md](https://github.com/woowoo-dev/wuff/blob/master/CHANGELOG.md).
