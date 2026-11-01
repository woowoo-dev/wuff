//
// Created by Michal Janecek on 01.02.2024.
//

#include "Linter.h"
#include <algorithm>

Linter::Linter(WooWooAnalyzer *analyzer) : Component(analyzer) {
    prepareQueries();
}

std::vector<Diagnostic> Linter::diagnose(const TextDocumentIdentifier &tdi) {

    auto doc = analyzer->getDocumentByUri(tdi.uri);

    std::vector<Diagnostic> diagnostics;
    diagnoseErrors(doc, diagnostics);
    diagnoseMissingNodes(doc, diagnostics);
    diagnoseUnresolvedReferences(doc, diagnostics);

    return diagnostics;
}

/**
 * Warn about reference values that point at nothing, but only where that is surely a mistake: when every
 * reference of the type has a complete target (Reference::isComplete, e.g. the bibliography) whose values
 * are known. A label may be defined in another textbook, so label references are never flagged.
 */
void Linter::diagnoseUnresolvedReferences(DialectedWooWooDocument *doc, std::vector<Diagnostic> &diagnostics) {
    auto project = analyzer->getProjectByDocument(doc);
    for (const ReferenceUsage &usage: doc->getReferenceUsages()) {
        if (usage.value.empty()) continue;  // nothing typed yet: a syntax error already
        auto references = DialectManager::getInstance()->getPossibleReferencesByTypeName(usage.typeName);
        if (references.empty() ||
            !std::all_of(references.begin(), references.end(), [](const Reference &ref) { return ref.isComplete(); }))
            continue;

        auto origin = project->describeReferencables(references.front());
        if (!origin.has_value() || project->resolve(references, usage.value).has_value()) continue;

        Range range = usage.range;
        doc->utfMappings->utf8ToUtf16(range);
        diagnostics.emplace_back(range, "'" + usage.value + "' is not in " + origin.value(), "wuff",
                                 DiagnosticSeverity::Warning);
    }
}


void Linter::diagnoseErrors(WooWooDocument *doc, std::vector<Diagnostic> &diagnostics) {

    TSQueryCursor *errorCursor = ts_query_cursor_new();
    ts_query_cursor_exec(errorCursor, queries[errorNodesQuery], ts_tree_root_node(doc->tree));

    TSQueryMatch match;
    while (ts_query_cursor_next_match(errorCursor, &match)) {
        for (unsigned i = 0; i < match.capture_count; ++i) {
            TSNode error_node = match.captures[i].node;
            auto text = doc->getNodeText(error_node);
            // Construct the range
            TSPoint start_point = ts_node_start_point(error_node);
            TSPoint end_point = ts_node_end_point(error_node);
            Range range = {Position{start_point.row, start_point.column}, Position{end_point.row, end_point.column}};
            doc->utfMappings->utf8ToUtf16(range);
            if (range.start.line != range.end.line) {
                range.end = Position{start_point.row, start_point.column + 1};
            }

            Diagnostic diagnostic = {range, "Syntax error", "source", DiagnosticSeverity::Error};
            diagnostics.emplace_back(diagnostic);
        }
    }

}

void Linter::diagnoseMissingNodes(WooWooDocument *doc, std::vector<Diagnostic> &diagnostics) {
    // Recursive lambda function to traverse the syntax tree
    std::function<void(TSNode)> traverseTree = [&](TSNode node) {
        uint32_t childCount = ts_node_child_count(node);

        for (uint32_t i = 0; i < childCount; ++i) {
            TSNode child = ts_node_child(node, i);

            if (ts_node_is_missing(child)) {
                // Constructing the range for the missing node
                TSPoint start_point = ts_node_start_point(child);
                TSPoint end_point = ts_node_end_point(child);

                Range range = {Position{start_point.row, start_point.column},
                               Position{end_point.row, end_point.column + 1}};
                doc->utfMappings->utf8ToUtf16(range);
                // Create the diagnostic for the missing node
                Diagnostic diagnostic = {range, "Syntax error: MISSING " + std::string(ts_node_type(child)), "source",
                                         DiagnosticSeverity::Error};
                diagnostics.emplace_back(diagnostic);
            }

            // Recursively traverse the child nodes
            traverseTree(child);
        }
    };

    // Start the tree traversal from the root node
    traverseTree(ts_tree_root_node(doc->tree));
}

const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>> &Linter::getQueryStringByName() const {
    return queryStringsByName;
}

const std::string Linter::errorNodesQuery = "errorNodesQuery";

const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>> Linter::queryStringsByName = {
        {errorNodesQuery, std::make_pair(tree_sitter_woowoo(), "(ERROR) @error")}
};