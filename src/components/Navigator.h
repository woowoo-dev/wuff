//
// Created by Michal Janecek on 31.01.2024.
//

#ifndef WUFF_NAVIGATOR_H
#define WUFF_NAVIGATOR_H

#include <string>
#include "../WooWooAnalyzer.h"
#include "Component.h"
#include "../lsp/LSPTypes.h"


class Navigator : Component {

public:
    explicit Navigator(WooWooAnalyzer *analyzer);

    Location goToDefinition(const DefinitionParams &params);

    std::vector<Location> references(const ReferenceParams &params);

    // the range of the value to rename, nullopt when there is nothing to rename or the target cannot be edited
    std::optional<Range> prepareRename(const TextDocumentPositionParams &params);

    WorkspaceEdit rename(const RenameParams &params);
    
    // document which was renamed (should be already updated) + its old path string
    WorkspaceEdit refactorDocumentReferences(const std::vector<std::pair<std::string, std::string>> & renamedDocuments);

private:
    Location navigateToFile(const DefinitionParams &params, const std::string &relativeFilePath);

    Location resolveShortInnerEnvironmentReference(const DefinitionParams &params, TSNode node);

    Location resolveShorthandReference(const std::string &shorthandType, const DefinitionParams &params, TSNode node);

    Location resolveMetaBlockReference(const DefinitionParams &params);

    Location findReference(const DefinitionParams &params, const std::vector<Reference> &possibleReferences,
                           const std::string &referencingValue);

    std::optional<std::pair<MetaContext *, std::pair<TSNode, TSNode>>>
    extractMetaFieldKeyValue(const TextDocumentIdentifier &tdi, const Position &p);

    std::vector<Location> findMetaBlockReferences(const ReferenceParams &params);

    std::vector<Location> findUsageReferences(const ReferenceParams &params, DialectedWooWooDocument *document,
                                              const ReferenceUsage &usage);

    void searchProjectForReferences(std::vector<Location> &locations, WooWooDocument *doc,
                                    const std::vector<Reference> &references, const std::string &referenceValue);

    [[nodiscard]] const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>> &
    getQueryStringByName() const override;

    static const std::string goToDefinitionQuery;
    static const std::string metaFieldQuery;
    static const std::string findReferencesQuery;
    static const std::string filenameQuery;
    static const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>> queryStringsByName;


};


#endif //WUFF_NAVIGATOR_H
