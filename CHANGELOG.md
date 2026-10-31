# Changelog

All notable changes to `wuff` are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

The changelog starts with version 1.3.0; earlier releases are documented in the commit
history only.

## [1.3.0] - 2026-10-24

### Added

- Semantic highlighting of anonymous (indented) math blocks, which the FIT math dialect
  renders as unnumbered equations. The highlighter reports the LaTeX elements of every
  `implicit_outer_environment` with standard LSP token types — `number` for plain math,
  `function` for commands, `operator` for `^ _ = < > + - * / & |` and `\\`, `string` for
  the argument of `\text{…}`, `type` for the environment name of `\begin{…}` and `comment`
  for `%` — so clients render such a block like inline math without any configuration.
  This works at document level, inside wobject bodies and inside `.itemize:`/`.item:`
  lists, at any nesting depth. Blocks nested in a math environment (`equation`, `align`,
  `gather`) are left to the client's own math highlighting.
- `Environment::math` and the dialect queries `Dialect::isMathEnvironment` /
  `Dialect::isImplicitOuterEnvironmentMath`, backed by a new per-environment `math: true`
  key in the dialect YAML.
- Richer hover hints: `DialectManager::getDescription` returns Markdown now. Below the
  description of the hovered document part, wobject or environment it lists the meta-block
  fields that the dialect accepts for it, marks the required ones and names the fragile
  form (`!tikz:`) of environments that need it. `Field` gained an optional `description`,
  which is appended to the field name in that list, so the dialect can document keys such
  as `aspect` (default `16x9`) without repeating itself in the entry description.
- `setup.py` lists the supported Python versions (3.10 to 3.14) and the supported platforms
  as PyPI classifiers; the package declared none before, so the project page showed no
  Python versions at all.

### Changed

- `Parser::extractStructureName` is public; the highlighter uses it to resolve the name of
  the enclosing outer environment.
- The test dialect copy (`tests/files/fit_math.yaml`) mirrors the FIT math dialect shipped
  with the language server (1.2.0). New fixtures `nested.woo`, `mathenv.woo` and
  `upstream.woo` cover anonymous math blocks, blocks nested in math environments, and the
  structures added in the dialect sync.
- The build asks for CMake 3.20 (it was 3.5, which is below the compatibility floor of
  CMake 4) and populates tree-sitter with `FetchContent_MakeAvailable()` instead of the
  deprecated `FetchContent_Populate()`. The build requirements are `setuptools>=68` and
  `cmake>=3.20`; the test extra asks for pytest 8 and the obsolete pytest-internals
  warning filter is gone.
- Wheels are built with cibuildwheel 4.3.4 (it was 2.21), so the released wheel set reaches
  CPython 3.14 instead of stopping at 3.13; the same major bump moved the default Linux
  image from manylinux2014 to manylinux_2_28 (glibc 2.28).
- Free-threaded CPython builds (`cp314t` and later) are skipped explicitly. cibuildwheel 4
  builds those variants by default now that free-threading is no longer experimental in
  3.14, but pybind11 2.11 cannot declare that the module is safe without the GIL: importing
  it re-enables the GIL with a `RuntimeWarning`, which the test configuration turns into an
  error. GIL-enabled 3.14 wheels are still published, and a free-threaded interpreter
  installs the sdist instead.
- macOS wheels are built for both architectures: `macos-15` (arm64) and `macos-15-intel`,
  the last x86_64 runner. The matrix produced arm64 wheels only before, so Intel Macs
  had to build from source. Both jobs moved off `macos-14`, which GitHub retires on
  2 November 2026.
- The macOS deployment target is no longer hard-coded in `src/CMakeLists.txt`. `setup.py`
  passes `MACOSX_DEPLOYMENT_TARGET` to CMake as `CMAKE_OSX_DEPLOYMENT_TARGET`, defaulting to
  10.15 — the first macOS whose libc++ provides `std::filesystem` — and
  `[tool.cibuildwheel.macos]` sets the same value for wheel builds, because cibuildwheel's
  own defaults (10.9 for 3.10 and 3.11, 10.13 for 3.12 and 3.13) are below what the module
  can compile against. delocate also rejects a wheel whose binaries require a newer macOS
  than the tag promises. Intel wheels stay `macosx_10_15_x86_64`, arm64 keeps its 11.0
  floor.
- The minimum Python is 3.10 instead of 3.8: 3.8 and 3.9 are past end of life, so
  `python_requires` is `>=3.10`, the `mypy` and `ruff` targets say 3.10 and the build skips
  the `cp39` wheels that pip would refuse to install anyway. Installations on 3.8 and 3.9
  are unaffected: pip keeps resolving them to 1.2.1, the last release that supports them.
- The obsolete `test-skip` entry for `*universal2:arm64` is gone. It only ever applied to
  `universal2` wheels, which this project does not build: each macOS runner produces a
  single-architecture wheel, so nothing was being skipped.

### Fixed

- yaml-cpp is pinned to `yaml-cpp-0.9.0` instead of being cloned from `master`, so a build
  is reproducible and cannot break overnight. 0.9.0 is also the first release that
  configures with CMake 4 — 0.8.0 declares `cmake_minimum_required(VERSION 3.4)`, which
  CMake 4 rejects outright.
- The highlighter no longer needs to guess which blocks are formulas from the shape of
  the surrounding text: whether an indented block is math is now decided by the dialect
  alone.
