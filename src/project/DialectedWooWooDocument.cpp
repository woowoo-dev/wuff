//
// Created by Michal Janecek on 08.02.2024.
//

#include "DialectedWooWooDocument.h"
#include <algorithm>
#include <tuple>
#include "../utils/utils.h"

DialectedWooWooDocument::DialectedWooWooDocument(const fs::path &documentPath1)
        : WooWooDocument(documentPath1) {
    prepareQueries();
    index();
}


DialectedWooWooDocument::~DialectedWooWooDocument() {
    ts_query_delete(fieldQuery);
    ts_query_delete(referencesQuery);
}

void DialectedWooWooDocument::prepareQueries() {
    uint32_t errorOffset;
    TSQueryError errorType;
    fieldQuery = ts_query_new(
            tree_sitter_yaml(),
            MetaContext::metaFieldQueryString.c_str(),
            MetaContext::metaFieldQueryString.size(),
            &errorOffset,
            &errorType
    );

    if (!fieldQuery) {
        utils::reportQueryError("fieldQuery", errorOffset, errorType);
    }

    referencesQuery = ts_query_new(
            tree_sitter_woowoo(),
            referencesQueryString.c_str(),
            referencesQueryString.size(),
            &errorOffset,
            &errorType
    );

    if (!referencesQuery) {
        utils::reportQueryError("fieldQuery", errorOffset, errorType);
    }
}

void DialectedWooWooDocument::index() {
    referencablesByNode.clear();
    referencableNodes.clear();
    for (const std::string &typeName: DialectManager::getInstance()->getReferencingTypeNames()) {
        for (const Reference &ref: DialectManager::getInstance()->getPossibleReferencesByTypeName(typeName)) {
            // only meta-field targets live in documents
            if (ref.target != ReferenceTarget::MetaField) continue;

            // create if not exist
            referencableNodes[ref];

            for (MetaContext *mx: metaBlocks) {
                if ((ref.structureType.empty() || ref.structureType == mx->parentType) &&
                    (ref.structureName.empty() || ref.structureName == mx->parentName)) {
                    // this metablock is matching the requiremens by the reference

                    TSQueryCursor *wooCursor = ts_query_cursor_new();
                    ts_query_cursor_exec(wooCursor, fieldQuery, ts_tree_root_node(mx->tree));

                    TSQueryMatch match;
                    while (ts_query_cursor_next_match(wooCursor, &match)) {
                        TSNode valueNode;
                        bool correctKey = false;
                        for (uint32_t i = 0; i < match.capture_count; ++i) {
                            uint32_t capture_index = match.captures[i].index;
                            TSNode capturedNode = match.captures[i].node;

                            uint32_t valueCaptureName;
                            const char *valueCaptureNameChars = ts_query_capture_name_for_id(
                                    fieldQuery, capture_index, &valueCaptureName);
                            std::string valueCaptureNameStr(valueCaptureNameChars, valueCaptureName);

                            if (valueCaptureNameStr == "value") {
                                // mark the value node
                                valueNode = capturedNode;
                            } else if (valueCaptureNameStr == "key") {
                                if (getMetaNodeText(mx, capturedNode) == ref.metaKey) {
                                    // the field key is what we want
                                    correctKey = true;
                                } else {
                                    break;
                                }
                            }
                        }
                        if (!correctKey) continue;
                        referencablesByNode[typeName].emplace_back(std::make_pair(mx, valueNode));
                        referencableNodes[ref][getMetaNodeText(mx, valueNode)] = std::make_pair(mx, valueNode);

                    }
                    ts_query_cursor_delete(wooCursor);
                }
            }
        }
    }
    collectReferenceUsages();
}

void DialectedWooWooDocument::collectReferenceUsages() {
    referenceUsages.clear();
    auto dialectManager = DialectManager::getInstance();
    auto isReferencing = [dialectManager](const std::string &typeName) {
        return !dialectManager->getPossibleReferencesByTypeName(typeName).empty();
    };
    auto rangeOf = [](TSNode node, uint32_t lineOffset = 0) {
        TSPoint s = ts_node_start_point(node);
        TSPoint e = ts_node_end_point(node);
        return Range{{s.row + lineOffset, s.column}, {e.row + lineOffset, e.column}};
    };

    // meta-block fields (example --> "ref: chapter-01")
    for (MetaContext *mx: metaBlocks) {
        TSQueryCursor *cursor = ts_query_cursor_new();
        ts_query_cursor_exec(cursor, fieldQuery, ts_tree_root_node(mx->tree));
        TSQueryMatch match;
        while (ts_query_cursor_next_match(cursor, &match)) {
            std::optional<TSNode> keyNode, valueNode;
            for (uint32_t i = 0; i < match.capture_count; ++i) {
                uint32_t length;
                const char *name = ts_query_capture_name_for_id(fieldQuery, match.captures[i].index, &length);
                if (std::string(name, length) == "key") {
                    keyNode = match.captures[i].node;
                } else {
                    valueNode = match.captures[i].node;
                }
            }
            if (!keyNode || !valueNode) continue;
            auto key = getMetaNodeText(mx, *keyNode);
            if (!isReferencing(key)) continue;
            referenceUsages.push_back({key, getMetaNodeText(mx, *valueNode), rangeOf(*valueNode, mx->lineOffset)});
        }
        ts_query_cursor_delete(cursor);
    }

    // short inner environments (example --> ".reference:chapter-01")
    // and shorthands (example --> "See Chapter 1"#chapter-01")
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, referencesQuery, ts_tree_root_node(tree));
    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        if (match.capture_count == 0) continue;
        TSNode node = match.captures[0].node;
        std::string nodeType = ts_node_type(node);

        std::string typeName;
        TSNode valueNode = node;
        if (nodeType == "short_inner_environment") {
            auto bodyNode = utils::getChild(node, "short_inner_environment_body");
            if (!bodyNode) continue;
            typeName = utils::getChildText(node, "short_inner_environment_type", this);
            valueNode = *bodyNode;
        } else {
            typeName = nodeType == "verbose_inner_environment_hash_end" ? "#" : "@";
        }
        if (!isReferencing(typeName)) continue;

        Range range = rangeOf(valueNode);
        if (ts_node_is_missing(valueNode)) {
            // nothing typed yet: an empty value right after the ':' (or '#', '@')
            TSNode previous = ts_node_prev_sibling(valueNode);
            if (!ts_node_is_null(previous)) {
                range.start = rangeOf(previous).end;
            }
            range.end = range.start;
            referenceUsages.push_back({typeName, "", range});
        } else {
            referenceUsages.push_back({typeName, getNodeText(valueNode), range});
        }
    }
    ts_query_cursor_delete(cursor);

    std::sort(referenceUsages.begin(), referenceUsages.end(), [](const ReferenceUsage &a, const ReferenceUsage &b) {
        return std::tie(a.range.start.line, a.range.start.character) <
               std::tie(b.range.start.line, b.range.start.character);
    });
}

const std::vector<ReferenceUsage> &DialectedWooWooDocument::getReferenceUsages() const {
    return referenceUsages;
}

std::optional<ReferenceUsage> DialectedWooWooDocument::findReferenceUsageAt(uint32_t line, uint32_t utf8Column) const {
    auto position = std::tie(line, utf8Column);
    for (const ReferenceUsage &usage: referenceUsages) {
        if (std::tie(usage.range.start.line, usage.range.start.character) <= position &&
            position <= std::tie(usage.range.end.line, usage.range.end.character)) {
            return usage;
        }
    }
    return std::nullopt;
}


std::vector<std::pair<MetaContext *, TSNode> > DialectedWooWooDocument::getReferencablesBy(
        const std::string &referencingTypeName) {
    auto refTypes = DialectManager::getInstance()->getReferencingTypeNames();
    if (std::find(refTypes.begin(), refTypes.end(), referencingTypeName) == refTypes.end()) {
        return {};
    }
    return referencablesByNode[referencingTypeName];
}

Referencable DialectedWooWooDocument::referencableOf(MetaContext *mx, TSNode valueNode) {
    TSPoint s = ts_node_start_point(valueNode);
    TSPoint e = ts_node_end_point(valueNode);
    Range range{{s.row + mx->lineOffset, s.column}, {e.row + mx->lineOffset, e.column}};
    utfMappings->utf8ToUtf16(range);
    return {getMetaNodeText(mx, valueNode), Location(utils::pathToUri(documentPath), range), "", ""};
}

std::vector<Referencable> DialectedWooWooDocument::getReferencables(const Reference &reference) {
    std::vector<Referencable> referencables;
    auto found = referencableNodes.find(reference);
    if (found == referencableNodes.end()) return referencables;
    for (const auto &valueAndNode: found->second) {
        referencables.push_back(referencableOf(valueAndNode.second.first, valueAndNode.second.second));
    }
    return referencables;
}

std::optional<Referencable>
DialectedWooWooDocument::findReferencable(const Reference &reference, const std::string &referenceValue) {
    auto found = referencableNodes.find(reference);
    if (found == referencableNodes.end()) return std::nullopt;
    auto valueAndNode = found->second.find(referenceValue);
    if (valueAndNode == found->second.end()) return std::nullopt;
    return referencableOf(valueAndNode->second.first, valueAndNode->second.second);
}

std::vector<Location>
DialectedWooWooDocument::findLocationsOfReferences(const Reference &reference, const std::string &referenceValue) {
    std::vector<Location> locations;
    for (const ReferenceUsage &usage: referenceUsages) {
        if (usage.value != referenceValue) continue;
        auto references = DialectManager::getInstance()->getPossibleReferencesByTypeName(usage.typeName);
        if (std::any_of(references.begin(), references.end(),
                        [&reference](const Reference &ref) { return ref.sharesTargetWith(reference); })) {
            locations.emplace_back(utils::pathToUri(documentPath), usage.range);
        }
    }
    return locations;
}

// constructs besides metablock fields which could reference something
const std::string DialectedWooWooDocument::referencesQueryString = R"(
(short_inner_environment) @type
(verbose_inner_environment_hash_end) @type
(verbose_inner_environment_at_end) @type
)";

void DialectedWooWooDocument::updateSource(std::string &source) {
    WooWooDocument::updateSource(source);
    prepareQueries();
    index();
}