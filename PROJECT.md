# WooWoo Project Support

## Overview

Starting with version 1.2.0, wuff supports **WooWoo Projects**. A project is a folder containing a `Woofile` at its root.

## Current Behavior

- **Project Detection**: Folders with a `Woofile` are treated as projects
- **Document Scoping**: Completions, references, and go-to-definition work within project boundaries
- **Multiple Projects**: A workspace can contain multiple projects
- **Standalone Files**: `.woo` files outside any project folder work independently

## Limitations

### Woofile Parsing
Since version 1.4.0 the `Woofile` is read, for one field so far: `builder.bibtex`, the project's BibTeX file (relative to the `Woofile`'s folder). Its entries are what the dialect's bibliography references (`.cite:`) complete, open, list and check. Other fields are ignored; new ones go to `src/project/Woofile.*`.

The `Woofile` and the `.bib` file are read again when their modification time changes, so editing them needs no restart. A missing or broken file means no bibliography and is reported on stderr.

### Dynamic Changes
Project structure is detected **at startup only**:
- Adding a new `Woofile` → **Requires IDE restart** to detect
- Deleting a `Woofile` → **Requires IDE restart** to update
- Moving files between projects → **Requires IDE restart**

### Workaround
If you modify your project structure, restart your IDE or reload the workspace to apply changes.

## Future Plans

- More `Woofile` fields (e.g. the `catalogue`, so labels from other textbooks can be resolved)
- Dynamic project detection (watch for new or deleted Woofiles)
