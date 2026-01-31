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
The `Woofile` content is **not parsed** in this version. The file only serves as a marker to identify project boundaries. Features like BibTeX integration are planned for future releases.

### Dynamic Changes
Project structure is detected **at startup only**:
- Adding a new `Woofile` → **Requires IDE restart** to detect
- Deleting a `Woofile` → **Requires IDE restart** to update
- Moving files between projects → **Requires IDE restart**

### Workaround
If you modify your project structure, restart your IDE or reload the workspace to apply changes.

## Future Plans

- Woofile YAML parsing
- BibTeX bibliography integration
- Dynamic project detection (watch for Woofile changes)
