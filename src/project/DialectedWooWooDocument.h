//
// Created by Michal Janecek on 08.02.2024.
//

#ifndef DIALECTED_WOOWOO_DOCUMENT_H
#define DIALECTED_WOOWOO_DOCUMENT_H

#include "WooWooDocument.h"
#include "../dialect/DialectManager.h"
#include <filesystem>
#include <optional>
#include "tree_sitter/api.h"
#include "../lsp/LSPTypes.h"
#include "ReferenceUsage.h"
#include "Referencable.h"

namespace fs = std::filesystem;

class DialectedWooWooDocument : public WooWooDocument {
public:
    
    DialectedWooWooDocument(const fs::path& documentPath1);

    
    ~DialectedWooWooDocument() override;
    std::vector<std::pair<MetaContext *, TSNode>> getReferencablesBy(const std::string& referencingTypeName);
    void updateSource(std::string &source) override;

    // the meta-field values the reference can point at (UTF-16 locations)
    std::vector<Referencable> getReferencables(const Reference & reference);
    std::optional<Referencable> findReferencable(const Reference & reference, const std::string & referenceValue);
    
    // usages whose value is referenceValue and whose type can refer to the target of reference; UTF-8 ranges
    std::vector<Location> findLocationsOfReferences(const Reference & reference, const std::string & referenceValue);

    // every reference usage of the document, in document order
    const std::vector<ReferenceUsage> & getReferenceUsages() const;
    // the usage whose value contains the position (its end included)
    std::optional<ReferenceUsage> findReferenceUsageAt(uint32_t line, uint32_t utf8Column) const;


private:

    void index();
    void collectReferenceUsages();
    Referencable referencableOf(MetaContext * mx, TSNode valueNode);

    void prepareQueries();
    TSQuery * fieldQuery;
    const static std::string referencesQueryString;
    TSQuery * referencesQuery;
    // given a typeName, get all nodes that can be referenced by that
    std::unordered_map<std::string, std::vector<std::pair<MetaContext *, TSNode>> > referencablesByNode;
    
    // given Reference and value (of a metablock field), store the node and MetaContext (this is what is being referenced, e.g. label value)
    std::unordered_map<Reference, std::unordered_map<std::string, std::pair<MetaContext *, TSNode>>> referencableNodes;

    std::vector<ReferenceUsage> referenceUsages;
};

#endif 
