# AGENTS.md

`wuff` is the C++ analysis engine of the WooWoo tooling, exposed to Python with pybind11 and
published on PyPI. The chain is:

wuff (PyPI) → [woowoo-language-server](https://github.com/woowoo-dev/woowoo-language-server)
(imports `wuff`) → [vscode-woowoo-lsp](https://github.com/woowoo-dev/vscode-woowoo-lsp)
(bundles the language server as a git submodule; users install `wuff` with pip).

## Build and test

```bash
pip install ".[test]"   # builds the C++ extension; rerun after every C++ change
pytest
```

- pytest turns warnings into errors (`pyproject.toml`).
- The *Wheels* workflow runs on every PR: it builds wheels for Linux, macOS and Windows and
  runs the tests against each of them.

## Layout

- `src/WooWooAnalyzer.*`: the entry point; `src/Bindings.cpp` defines the Python API.
- `src/components/`: one class per LSP feature (completion, folding, highlighting, hover,
  diagnostics, navigation).
- `src/dialect/`: loading the dialect YAML; `src/project/`: documents, projects (`Woofile`,
  `Bibliography`), reference usages and UTF-8 to UTF-16 position mapping (LSP positions are
  UTF-16).
- `src/tree-sitter/`: generated parsers. Do not edit them by hand; the WooWoo grammar lives in
  the `tree-sitter-woowoo` repository.
- `tests/files/fit_math.yaml`: a copy of the language server's `dialects/fit_math.yaml`.

## Rules

- The language server calls the Python API in `src/Bindings.cpp`. A change of a method name
  or signature breaks the server, so change the server in the same go and say so in both
  changelogs.
- When the dialect changes in the language server, update `tests/files/fit_math.yaml` too.
- Reference features (completion, definition, references, rename, hover, warnings) start from
  the reference usages of a document (`DialectedWooWooDocument::findReferenceUsageAt`) and ask
  the project what a reference points at (`WooWooProject::resolve`, `getReferencables`). A new
  kind of target is a new `ReferenceTarget` handled there, not in the components; no code
  knows names from the dialect such as `cite`.
- Record user-visible changes under `## [Unreleased]` in `CHANGELOG.md`.
- Do not create releases or tags. Releases are made by a maintainer (see README, *Releasing*).
