#ifndef WUFF_REFERENCEUSAGE_H
#define WUFF_REFERENCEUSAGE_H

#include <string>
#include "../lsp/LSPTypes.h"

// A value that refers to something, e.g. `ct1` in `.reference:ct1`, in `"Chapter"#ct1` or in `ref: ct1`.
// What it can refer to is given by the dialect references of its type.
struct ReferenceUsage {
    // the referencing type: a short inner environment type, "#", "@" or a meta-field key
    std::string typeName;
    std::string value;
    // the range of the value (empty while the value is not typed yet); UTF-8 columns, document lines
    Range range;
};

#endif //WUFF_REFERENCEUSAGE_H
