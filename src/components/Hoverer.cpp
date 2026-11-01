//
// Created by Michal Janecek on 28.01.2024.
//

#include "Hoverer.h"
#include "../utils/utils.h"


Hoverer::Hoverer(WooWooAnalyzer *analyzer) : Component(analyzer) {
    prepareQueries();
}


std::string Hoverer::hover(const TextDocumentPositionParams &params) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto pos = document->utfMappings->utf16ToUtf8(params.position.line, params.position.character);

    // on a reference value: what it points at, when there is something to tell (e.g. a bibliography entry)
    auto usage = document->findReferenceUsageAt(pos.first, pos.second);
    if (usage.has_value()) {
        auto target = analyzer->getProjectByDocument(document)->resolve(
                DialectManager::getInstance()->getPossibleReferencesByTypeName(usage->typeName), usage->value);
        if (target.has_value() && !target->documentation.empty()) {
            return target->documentation;
        }
    }


    TSQueryCursor *cursor = ts_query_cursor_new();
    TSPoint start_point = {pos.first, pos.second};
    TSPoint end_point = {pos.first, pos.second + 1};
    ts_query_cursor_set_point_range(cursor, start_point, end_point);
    ts_query_cursor_exec(cursor, queries[hoverableNodesQuery], ts_tree_root_node(document->tree));

    TSQueryMatch match;
    std::string nodeType;
    std::string nodeText;
    if (ts_query_cursor_next_match(cursor, &match)) {
        if (match.capture_count > 0) {
            TSNode node = match.captures[0].node;
            nodeType = ts_node_type(node);
            nodeText = document->getNodeText(node);

        }
    }

    ts_query_cursor_delete(cursor);
    return DialectManager::getInstance()->getDescription(nodeType, nodeText);
}

const std::unordered_map <std::string, std::pair<TSLanguage *, std::string>> &Hoverer::getQueryStringByName() const {
    return queryStringsByName;
}

const std::string Hoverer::hoverableNodesQuery = "hoverableNodesQuery";
const std::unordered_map <std::string, std::pair<TSLanguage *, std::string>> Hoverer::queryStringsByName = {
        {hoverableNodesQuery, std::make_pair(tree_sitter_woowoo(), R"(
(document_part_type) @node
(wobject_type) @node
(short_inner_environment_type) @node
(verbose_inner_environment_type) @node
(outer_environment_type) @node
)")}};
