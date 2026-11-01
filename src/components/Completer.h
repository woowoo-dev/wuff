//
// Created by Michal Janecek on 31.01.2024.
//

#ifndef WUFF_COMPLETER_H
#define WUFF_COMPLETER_H

#include "../WooWooAnalyzer.h"
#include "../lsp/LSPTypes.h"
#include "Component.h"

#include <vector>

class Completer : Component {

public:
    explicit Completer(WooWooAnalyzer *analyzer);
    std::vector<CompletionItem> complete(const CompletionParams & params);
    
private:
    void completeInclude(std::vector<CompletionItem> & completionItems, const CompletionParams & params);
    void completeInnerEnvs(std::vector<CompletionItem> & completionItems, DialectedWooWooDocument * document,
                           uint32_t line, uint32_t character);
    void completeShorthand(std::vector<CompletionItem> & completionItems, const CompletionParams & params);
    void searchProjectForReferencables(std::vector<CompletionItem> & completionItems, WooWooDocument * doc, std::string & referencingValue);
    void completeReferenceUsage(std::vector<CompletionItem> & completionItems, DialectedWooWooDocument * document,
                                const ReferenceUsage & usage);

    [[nodiscard]] const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>>& getQueryStringByName() const override;
    
    static const std::string includeCollisionQuery;
    static const std::string shortInnerEnvironmentQuery;
    static const std::unordered_map<std::string, std::pair<TSLanguage*,std::string>> queryStringsByName;
    
};


#endif //WUFF_COMPLETER_H
