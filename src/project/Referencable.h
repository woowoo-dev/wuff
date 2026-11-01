#ifndef WUFF_REFERENCABLE_H
#define WUFF_REFERENCABLE_H

#include <string>
#include "../lsp/LSPTypes.h"

// Something a reference can point at: a label value in a document, an entry of the bibliography, ...
struct Referencable {
    std::string value;
    // UTF-16 (LSP) location of the value
    Location location;
    // one line describing it; empty when there is nothing to add to the value
    std::string detail;
    // Markdown; empty when there is nothing to add to the value
    std::string documentation;
};

#endif //WUFF_REFERENCABLE_H
