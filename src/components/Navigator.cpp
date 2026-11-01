//
// Created by Michal Janecek on 31.01.2024.
//

#include "Navigator.h"

#include "../utils/utils.h"
#include <algorithm>  // Include for std::find_if

Navigator::Navigator(WooWooAnalyzer *analyzer) : Component(analyzer) {
    prepareQueries();
}

// - - REFERENCES

std::vector<Location> Navigator::references(const ReferenceParams &params) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto pos = document->utfMappings->utf16ToUtf8(params.position.line, params.position.character);
    uint32_t line = pos.first;
    uint32_t character = pos.second;

    auto usage = document->findReferenceUsageAt(line, character);
    if (usage.has_value()) {
        return findUsageReferences(params, document, usage.value());
    }

    TSQueryCursor *cursor = ts_query_cursor_new();
    TSPoint start_point = {line, character};
    TSPoint end_point = {line, character + 1};
    ts_query_cursor_set_point_range(cursor, start_point, end_point);
    ts_query_cursor_exec(cursor, queries[findReferencesQuery], ts_tree_root_node(document->tree));

    TSQueryMatch match;
    std::string nodeType;
    std::string nodeText;
    if (ts_query_cursor_next_match(cursor, &match)) {
        if (match.capture_count > 0) {
            TSNode node = match.captures[0].node;

            nodeType = ts_node_type(node);
            nodeText = document->getNodeText(node);

            if (nodeType == "meta_block") {
                // user executed Find All References on a meta-block
                return findMetaBlockReferences(params);
            }

        }
    }

    return {};

}

std::vector<Location> Navigator::findMetaBlockReferences(const ReferenceParams &params) {
    std::vector<Location> locations;

    auto metaFieldContext = extractMetaFieldKeyValue(params.textDocument, params.position);
    if (metaFieldContext.has_value()) {

        auto document = analyzer->getDocumentByUri(params.textDocument.uri);
        auto mx = metaFieldContext->first;
        auto keyNode = metaFieldContext->second.first;
        auto valueNode = metaFieldContext->second.second;

        auto s = ts_node_start_point(valueNode);
        auto e = ts_node_end_point(valueNode);
        
        auto metaKey = document->getMetaNodeText(mx, keyNode);

        // check if we should include declaration, and if this metaKey is referencable (dialect-specific behaviour)
        if (params.includeDeclaration &&
            std::any_of(DialectManager::getInstance()->allReferences.begin(),
                        DialectManager::getInstance()->allReferences.end(),
                        [&metaKey](const Reference& ref) { return ref.metaKey == metaKey; })) {

            Location l = {utils::pathToUri(document->documentPath), Range{{s.row + mx->lineOffset, s.column},
                                                                          {e.row + mx->lineOffset, e.column}}};
            document->utfMappings->utf8ToUtf16(l);
            locations.emplace_back(l);
        }

        searchProjectForReferences(locations, document, {Reference(metaKey)},
                                   document->getMetaNodeText(mx, valueNode));
        return locations;
    } else {
        return {};
    }
}

// all usages of the value that point at the same target as the usage; with the declaration (a label or a
// bibliography entry) first
std::vector<Location> Navigator::findUsageReferences(const ReferenceParams &params, DialectedWooWooDocument *document,
                                                     const ReferenceUsage &usage) {
    std::vector<Location> locations;
    auto references = DialectManager::getInstance()->getPossibleReferencesByTypeName(usage.typeName);
    if (params.includeDeclaration) {
        auto declaration = analyzer->getProjectByDocument(document)->resolve(references, usage.value);
        if (declaration.has_value()) {
            locations.emplace_back(declaration->location);
        }
    }
    searchProjectForReferences(locations, document, references, usage.value);
    return locations;
}

void
Navigator::searchProjectForReferences(std::vector<Location> &locations, WooWooDocument *doc,
                                      const std::vector<Reference> &references, const std::string &referenceValue) {

    for (auto projectDocument: analyzer->getProjectByDocument(doc)->getAllDocuments()) {
        for (auto refLocation: projectDocument->findLocationsOfReferences(references, referenceValue)) {
            projectDocument->utfMappings->utf8ToUtf16(refLocation);
            locations.emplace_back(refLocation);
        }
    }
}

// - - RENAME

std::optional<Range> Navigator::prepareRename(const TextDocumentPositionParams &params) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto pos = document->utfMappings->utf16ToUtf8(params.position.line, params.position.character);

    std::optional<Range> range;
    auto usage = document->findReferenceUsageAt(pos.first, pos.second);
    if (usage.has_value()) {
        auto references = DialectManager::getInstance()->getPossibleReferencesByTypeName(usage->typeName);
        if (usage->value.empty() ||
            !std::all_of(references.begin(), references.end(), [](const Reference &ref) { return ref.isRenamable(); })) {
            return std::nullopt;
        }
        range = usage->range;
    } else if (document->getMetaContextByLine(pos.first)) {
        // a declaration, e.g. `label: ct1`
        auto metaFieldContext = extractMetaFieldKeyValue(params.textDocument, params.position);
        if (!metaFieldContext.has_value()) return std::nullopt;
        auto mx = metaFieldContext->first;
        auto metaKey = document->getMetaNodeText(mx, metaFieldContext->second.first);
        auto &allReferences = DialectManager::getInstance()->allReferences;
        if (std::none_of(allReferences.begin(), allReferences.end(),
                         [&metaKey](const Reference &ref) { return ref.isRenamable() && ref.metaKey == metaKey; })) {
            return std::nullopt;
        }
        auto valueNode = metaFieldContext->second.second;
        auto s = ts_node_start_point(valueNode);
        auto e = ts_node_end_point(valueNode);
        range = Range{{s.row + mx->lineOffset, s.column}, {e.row + mx->lineOffset, e.column}};
    } else {
        return std::nullopt;
    }

    document->utfMappings->utf8ToUtf16(range.value());
    return range;
}

WorkspaceEdit Navigator::rename(const RenameParams &params) {
    if (!prepareRename(params).has_value()) {
        return {};
    }

    // First, get all references of the symbol.
    ReferenceParams rp = ReferenceParams(params.textDocument, params.position, true);
    auto referencesLocations = references(rp);

    WorkspaceEdit we;
    for (const auto &refLoc: referencesLocations) {
        TextEdit te = TextEdit(refLoc.range, params.newName);
        we.add_change(refLoc.uri, te);
    }

    return we;
}

// - - GO TO DEFINITION
Location Navigator::goToDefinition(const DefinitionParams &params) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto pos = document->utfMappings->utf16ToUtf8(params.position.line, params.position.character);
    uint32_t line = pos.first;
    uint32_t character = pos.second;

    // in a reference value (its end included)
    auto usage = document->findReferenceUsageAt(line, character);
    if (usage.has_value()) {
        return findReference(params, DialectManager::getInstance()->getPossibleReferencesByTypeName(usage->typeName),
                             usage->value);
    }

    TSQueryCursor *cursor = ts_query_cursor_new();
    TSPoint start_point = {line, character};
    TSPoint end_point = {line, character + 1};
    ts_query_cursor_set_point_range(cursor, start_point, end_point);
    ts_query_cursor_exec(cursor, queries[goToDefinitionQuery], ts_tree_root_node(document->tree));

    TSQueryMatch match;
    std::string nodeType;
    std::string nodeText;
    if (ts_query_cursor_next_match(cursor, &match)) {
        if (match.capture_count > 0) {
            TSNode node = match.captures[0].node;

            nodeType = ts_node_type(node);
            nodeText = document->getNodeText(node);

            if (nodeType == "filename") {
                return navigateToFile(params, nodeText);
            }
            if (nodeType == "short_inner_environment") {
                return resolveShortInnerEnvironmentReference(params, node);
            }
            if (nodeType == "verbose_inner_environment_hash_end") {
                return resolveShorthandReference("#", params, node);
            }
            if (nodeType == "verbose_inner_environment_at_end") {
                return resolveShorthandReference("@", params, node);
            }
            if (nodeType == "meta_block") {
                return resolveMetaBlockReference(params);
            }

        }
    }
    return Location("", Range{Position{0, 0}, Position{0, 0}});
}

Location Navigator::navigateToFile(const DefinitionParams &params, const std::string &relativeFilePath) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto fileBegin = Range{Position{0, 0}, Position{0, 0}};
    std::error_code error;
    fs::path filePath = fs::canonical(document->documentPath.parent_path() / relativeFilePath, error);
    if (error) {
        // the included file does not exist
        return {"", fileBegin};
    }
    auto fileUri = "file://" + filePath.generic_string();
    return {fileUri, fileBegin};
}


Location Navigator::resolveShortInnerEnvironmentReference(const DefinitionParams &params, TSNode node) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto shortInnerEnvironmentType = utils::getChildText(node, "short_inner_environment_type", document);

    // obtain what can be referenced by this environment
    std::vector<Reference> referenceTargets = DialectManager::getInstance()->getPossibleReferencesByTypeName(
            shortInnerEnvironmentType);

    // obtain the body part of the referencing environment 
    auto value = utils::getChildText(node, "short_inner_environment_body", document);

    return findReference(params, referenceTargets, value);
}

Location
Navigator::resolveShorthandReference(const std::string &shorthandType, const DefinitionParams &params, TSNode node) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);

    // obtain what can be referenced by this environment
    std::vector<Reference> referenceTargets = DialectManager::getInstance()->getPossibleReferencesByTypeName(shorthandType);

    return findReference(params, referenceTargets, document->getNodeText(node));
}


std::optional<std::pair<MetaContext *, std::pair<TSNode, TSNode>>>
Navigator::extractMetaFieldKeyValue(const TextDocumentIdentifier &tdi, const Position &p) {
    auto document = analyzer->getDocumentByUri(tdi.uri);
    auto pos = document->utfMappings->utf16ToUtf8(p.line, p.character);
    uint32_t line = pos.first;
    uint32_t character = pos.second;
    TSQueryCursor *cursor = ts_query_cursor_new();
    MetaContext *mx = document->getMetaContextByLine(line);
    // points adjusted by metablock position
    TSPoint start_point = {line - mx->lineOffset, character};
    TSPoint end_point = {line - mx->lineOffset, character + 1};
    ts_query_cursor_set_point_range(cursor, start_point, end_point);
    ts_query_cursor_exec(cursor, queries[metaFieldQuery], ts_tree_root_node(mx->tree));

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        TSNode metaFieldName;
        TSNode metaFieldValue;
        bool foundValue = false;
        bool foundKey = false;
        for (uint32_t i = 0; i < match.capture_count; ++i) {
            TSNode capturedNode = match.captures[i].node;
            uint32_t capture_id = match.captures[i].index;

            uint32_t capture_name_length;
            const char *capture_name_chars = ts_query_capture_name_for_id(queries[metaFieldQuery], capture_id,
                                                                          &capture_name_length);
            std::string capture_name(capture_name_chars, capture_name_length);
            if (capture_name == "key") {
                foundValue = true;
                metaFieldName = capturedNode;
            } else if (capture_name == "value") {
                foundKey = true;
                metaFieldValue = capturedNode;
            }
        }
        if (foundKey && foundValue) {
            ts_query_cursor_delete(cursor);
            return std::make_pair(mx, std::make_pair(metaFieldName, metaFieldValue));
        }
    }

    ts_query_cursor_delete(cursor);
    return std::nullopt;
}

Location Navigator::resolveMetaBlockReference(const DefinitionParams &params) {
    auto metaFieldContext = extractMetaFieldKeyValue(params.textDocument, params.position);
    if (metaFieldContext.has_value()) {
        auto mx = metaFieldContext->first;
        auto keyNode = metaFieldContext->second.first;
        auto valueNode = metaFieldContext->second.second;
        auto document = analyzer->getDocumentByUri(params.textDocument.uri);
        return findReference(params, DialectManager::getInstance()->getPossibleReferencesByTypeName(
                                     document->getMetaNodeText(mx, keyNode)),
                             document->getMetaNodeText(mx, valueNode));
    } else {
        return Location("", Range{Position{0, 0}, Position{0, 0}});

    }
}


Location Navigator::findReference(const DefinitionParams &params, const std::vector<Reference> &possibleReferences,
                                  const std::string &referencingValue) {
    auto document = analyzer->getDocumentByUri(params.textDocument.uri);
    auto found = analyzer->getProjectByDocument(document)->resolve(possibleReferences, referencingValue);
    if (found.has_value()) {
        return found->location;
    }
    return Location("", Range{Position{0, 0}, Position{0, 0}});
}


// - - - - - - -

WorkspaceEdit Navigator::refactorDocumentReferences(const std::vector<std::pair<std::string, std::string>> & renamedDocuments) {
    WorkspaceEdit we;
    for (const auto &documentRename: renamedDocuments) {
        // document that was renamed (internally, in analyzer, the path should be already updated)
        WooWooDocument *renamedDoc = analyzer->getDocument(documentRename.second);
        std::string oldFileName = documentRename.first;
        fs::path oldFilePath(oldFileName);
        fs::path oldFileDir = oldFilePath.parent_path();
        
        // iterate over every document from the same project and look for references (from include statements)
        for (auto projectDocument: analyzer->getProjectByDocument(renamedDoc)->getAllDocuments()) {
            TSQueryCursor *cursor = ts_query_cursor_new();
            ts_query_cursor_exec(cursor, queries[filenameQuery], ts_tree_root_node(projectDocument->tree));
            TSQueryMatch match;
            std::string nodeType;
            std::string nodeText;
            
            while (ts_query_cursor_next_match(cursor, &match)) {
                if (match.capture_count > 0) {
                    TSNode node = match.captures[0].node;
                    nodeText = projectDocument->getNodeText(node);

                    fs::path includedFilePath(nodeText);
                    if (!includedFilePath.is_absolute()) {
                        includedFilePath = projectDocument->documentPath.parent_path() / includedFilePath;
                    }

                    try {
                        if(fs::canonical(oldFilePath) == fs::canonical(includedFilePath)){
                            // found include referring to the old filename
                            // we want to replace it with the new filename
                            
                            fs::path relativePath = fs::relative(renamedDoc->documentPath, projectDocument->documentPath.parent_path());

                            auto s = ts_node_start_point(node);
                            auto e = ts_node_end_point(node);

                            Location l = {utils::pathToUri(projectDocument->documentPath), Range{{s.row, s.column},
                                                                                              {e.row, e.column}}};
                            projectDocument->utfMappings->utf8ToUtf16(l);
                            
                            TextEdit te = TextEdit(l.range, relativePath.generic_string());
                            we.add_change(l.uri, te);
                        }

                    } catch (...) {
                        // if something fails, simply do not refactor it
                    }
                }
            }
        }
    }
    return we;
}

// - - - - - - -

const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>> &Navigator::getQueryStringByName() const {
    return queryStringsByName;
}

const std::string Navigator::metaFieldQuery = "metaFieldQuery";
const std::string Navigator::goToDefinitionQuery = "goToDefinitionQuery";
const std::string Navigator::findReferencesQuery = "findReferencesQuery";
const std::string Navigator::filenameQuery = "filenameQuery";
const std::unordered_map<std::string, std::pair<TSLanguage *, std::string>> Navigator::queryStringsByName = {
        {metaFieldQuery,      std::make_pair(tree_sitter_yaml(), MetaContext::metaFieldQueryString)},
        {goToDefinitionQuery, std::make_pair(tree_sitter_woowoo(),
                                             R"(
(filename) @type
(short_inner_environment) @type
(verbose_inner_environment_hash_end) @type
(verbose_inner_environment_at_end) @type
(meta_block) @type
)")},
        {findReferencesQuery, std::make_pair(tree_sitter_woowoo(),
                                             R"(
(meta_block) @type
)")},
        {filenameQuery,       std::make_pair(tree_sitter_woowoo(), "(filename) @filename")}
};


