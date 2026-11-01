#ifndef WUFF_BIBLIOGRAPHY_H
#define WUFF_BIBLIOGRAPHY_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../lsp/LSPTypes.h"

namespace fs = std::filesystem;

struct BibliographyEntry {
    std::string key;
    // entry type in lower case, e.g. "book"
    std::string type;
    // field values without the outer braces or quotes, whitespace collapsed; empty when missing
    std::string author;
    std::string title;
    std::string year;
    // UTF-16 (LSP) range of the key in the .bib file
    Range keyRange;

    // e.g. "Trefethen, Bau (1997): Numerical Linear Algebra"
    [[nodiscard]] std::string summary() const;
};

// The entries of a BibTeX file. @string macros and crossref are not resolved.
class Bibliography {
public:
    // nullptr when the file cannot be read; @comment, @string, @preamble and entries without a key are left out
    static std::unique_ptr<Bibliography> load(const fs::path &bibFile);

    [[nodiscard]] const BibliographyEntry *find(const std::string &key) const;
    [[nodiscard]] const std::vector<BibliographyEntry> &entries() const;
    [[nodiscard]] const fs::path &path() const;

private:
    Bibliography(fs::path filePath, const std::string &source);

    fs::path filePath;
    std::vector<BibliographyEntry> entryList;
    std::unordered_map<std::string, size_t> entryIndexByKey;
};

#endif //WUFF_BIBLIOGRAPHY_H
