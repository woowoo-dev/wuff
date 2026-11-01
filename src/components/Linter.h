//
// Created by Michal Janecek on 01.02.2024.
//

#ifndef WUFF_LINTER_H
#define WUFF_LINTER_H

#include "../WooWooAnalyzer.h"
#include "Component.h"
#include "../lsp/LSPTypes.h"
#include <vector>

class Linter : Component {

public:
    explicit Linter(WooWooAnalyzer *analyzer);
    std::vector<Diagnostic> diagnose(const TextDocumentIdentifier & tdi);

private:
    void diagnoseErrors(WooWooDocument * doc, std::vector<Diagnostic> & d);
    void diagnoseMissingNodes(WooWooDocument * doc, std::vector<Diagnostic> & d);
    void diagnoseUnresolvedReferences(DialectedWooWooDocument * doc, std::vector<Diagnostic> & d);

    [[nodiscard]] const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>>& getQueryStringByName() const override;

    static const std::string errorNodesQuery;
    static const std::unordered_map<std::string, std::pair<TSLanguage*,std::string>> queryStringsByName;
    
};


#endif //WUFF_LINTER_H
