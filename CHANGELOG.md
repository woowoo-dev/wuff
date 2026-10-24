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

### Changed

- `Parser::extractStructureName` is public; the highlighter uses it to resolve the name of
  the enclosing outer environment.
- The test dialect copy (`tests/files/fit_math.yaml`) mirrors the FIT math dialect shipped
  with the language server (1.2.0). New fixtures `nested.woo`, `mathenv.woo` and
  `upstream.woo` cover anonymous math blocks, blocks nested in math environments, and the
  structures added in the dialect sync.

### Fixed

- The highlighter no longer needs to guess which blocks are formulas from the shape of
  the surrounding text: whether an indented block is math is now decided by the dialect
  alone.
