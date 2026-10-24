//
// Created by Michal Janecek on 30.01.2024.
//

#include "Highlighter.h"
#include "../utils/utils.h"
#include <algorithm>
#include <utility>


Highlighter::Highlighter(WooWooAnalyzer *analyzer) : Component(analyzer) {
    prepareQueries();
}


std::vector<int> Highlighter::semanticTokens(const TextDocumentIdentifier &tdi) {
    auto documentPath = utils::uriToPathString(tdi.uri);
    auto document = analyzer->getDocument(documentPath);

    std::vector<int> data;
    std::vector<NodeInfo> nodes;


    TSQueryCursor *wooCursor = ts_query_cursor_new();
    ts_query_cursor_exec(wooCursor, queries[woowooHighlightQuery], ts_tree_root_node(document->tree));

    TSQueryMatch match;
    while (ts_query_cursor_next_match(wooCursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; ++i) {
            uint32_t capture_index = match.captures[i].index;
            TSNode capturedNode = match.captures[i].node;

            uint32_t capture_name_length;
            const char *capture_name_chars = ts_query_capture_name_for_id(queries[woowooHighlightQuery], capture_index,
                                                                          &capture_name_length);
            std::string capture_name(capture_name_chars, capture_name_length);

            TSPoint start_point = ts_node_start_point(capturedNode);
            TSPoint end_point = ts_node_end_point(capturedNode);

            nodes.emplace_back(start_point, end_point, capture_name);
        }
    }
    ts_query_cursor_delete(wooCursor);


    // - - Adding nodes from implicit (indented) math blocks
    addImplicitMathNodes(document, nodes);

    // - - Adding nodes from YAML highlights
    addMetaBlocksNodes(document, nodes);

    addCommentNodes(document, nodes);

    // Filtering
    std::vector<NodeInfo> unique;
    std::unordered_map<std::pair<int, int>, bool, pairHash> seen;
    for (const NodeInfo& nodeInfo: nodes) {
        std::pair<int, int> startPoint = document->utfMappings->utf8ToUtf16(nodeInfo.startPoint.row,
                                                                            nodeInfo.startPoint.column);
        if (seen.find(startPoint) == seen.end()) {
            seen[startPoint] = true;
            unique.emplace_back(nodeInfo);
        } else {
            // a different node with the same start was already processed
            // this occcurs because of the way the YAML queries are structured
            continue;
        }
    }
    nodes = std::move(unique);

    std::sort(nodes.begin(), nodes.end(), [](const NodeInfo &a, const NodeInfo &b) {
        // First, compare by line
        if (a.startPoint.row != b.startPoint.row) {
            return a.startPoint.row < b.startPoint.row;
        }
        // If lines are the same, compare by column
        return a.startPoint.column < b.startPoint.column;
    });

    uint32_t lastLine = 0;
    uint32_t lastStart = 0;
    for (const NodeInfo &node: nodes) {

        std::pair<uint32_t , uint32_t> startPoint = document->utfMappings->utf8ToUtf16(node.startPoint.row,
                                                                            node.startPoint.column);
        std::pair<uint32_t, uint32_t> endPoint = document->utfMappings->utf8ToUtf16(node.endPoint.row, node.endPoint.column);

        uint32_t deltaStart =
                lastLine == startPoint.first ? startPoint.second - lastStart : startPoint.second;

        uint32_t deltaLine = startPoint.first - lastLine;
        lastLine = startPoint.first;
        lastStart = startPoint.second;

        data.emplace_back(deltaLine);
        data.emplace_back(deltaStart);
        data.emplace_back(endPoint.second - startPoint.second);
        data.emplace_back(tokenTypeIndices[node.name]);
        data.emplace_back(0);
    }


    return data;
}


void Highlighter::addMetaBlocksNodes(WooWooDocument *document, std::vector<NodeInfo> &nodes) {


    for (MetaContext *metaContext: document->metaBlocks) {
        TSQueryCursor *yamlCursor = ts_query_cursor_new();
        ts_query_cursor_exec(yamlCursor, queries[yamlHighlightQuery], ts_tree_root_node(metaContext->tree));

        TSQueryMatch match;
        while (ts_query_cursor_next_match(yamlCursor, &match)) {
            for (uint32_t i = 0; i < match.capture_count; ++i) {
                TSNode capturedNode = match.captures[i].node;
                // The capture ID uniquely identifies the capture within the query
                uint32_t capture_id = match.captures[i].index;

                // Get the capture name using the capture ID
                uint32_t capture_name_length;
                const char *capture_name_chars = ts_query_capture_name_for_id(queries[yamlHighlightQuery], capture_id,
                                                                              &capture_name_length);
                std::string capture_name(capture_name_chars, capture_name_length);

                TSPoint start_point = ts_node_start_point(capturedNode);
                start_point.row += metaContext->lineOffset;
                TSPoint end_point = ts_node_end_point(capturedNode);
                end_point.row += metaContext->lineOffset;


                nodes.emplace_back(start_point, end_point, capture_name);
            }
        }

        ts_query_cursor_delete(yamlCursor);
    }

}

void Highlighter::addCommentNodes(WooWooDocument *document, std::vector<NodeInfo> &nodes) {

    for (CommentLine *cl: document->commentLines) {
        TSPoint start = {cl->lineNumber, 0};
        TSPoint end = {cl->lineNumber, cl->lineLength};
        nodes.emplace_back(start, end, "comment");
    }

}

namespace {

    bool isAsciiLetter(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    bool isLatexScriptChar(char c) {
        return c == '^' || c == '_';
    }

    bool isLatexOperatorChar(char c) {
        switch (c) {
            case '=':
            case '<':
            case '>':
            case '+':
            case '-':
            case '*':
            case '/':
            case '&':
            case '|':
                return true;
            default:
                return false;
        }
    }

    // An implicit outer environment also occurs inside the body of an explicit one, when that body
    // is indented deeper than the body itself. Only the math environments (`.equation:`, `.align:`,
    // `.gather:` in the FIT math dialect) are covered by the client: the TextMate `mathBlock` rule
    // colours every line of their body. Reporting those again would duplicate the highlighting,
    // while an indented block inside `.item:`, `.itemize:` or a wobject is a formula of its own.
    bool isInMathEnvironment(WooWooDocument *document, TSNode node) {
        DialectManager *dialectManager = DialectManager::getInstance();
        TSNode parent = ts_node_parent(node);
        while (!ts_node_is_null(parent)) {
            std::string parentType = ts_node_type(parent);
            if (parentType == "classic_outer_environment") {
                std::string environment = Parser::extractStructureName(parent, document->source);
                if (dialectManager->activeDialect->isMathEnvironment(environment)) {
                    return true;
                }
            }
            // inside a meta block there is no WooWoo source to highlight
            if (parentType == "meta_block" || parentType == "document_part" || parentType == "source_file") {
                return false;
            }
            parent = ts_node_parent(parent);
        }
        return false;
    }

    bool isLineInMetaBlock(WooWooDocument *document, uint32_t line) {
        for (const MetaContext *metaContext: document->metaBlocks) {
            TSNode metaRoot = ts_tree_root_node(metaContext->tree);
            uint32_t startByte = metaContext->byteOffset;
            uint32_t endByte = startByte + ts_node_end_byte(metaRoot);
            uint32_t startLine = metaContext->lineOffset;
            uint32_t endLine = startLine + ts_node_end_point(metaRoot).row;
            if (line < startLine || line > endLine) {
                continue;
            }
            // a meta block can start in the middle of a line (right after the header text),
            // so only lines that really contain a part of it count
            uint32_t position = startByte;
            for (uint32_t skipped = line - startLine; skipped > 0; --skipped) {
                while (position < endByte && document->source[position] != '\n') {
                    ++position;
                }
                if (position >= endByte) {
                    break;
                }
                ++position;
            }
            while (position < endByte &&
                   (document->source[position] == ' ' || document->source[position] == '\t' ||
                    document->source[position] == '\r')) {
                ++position;
            }
            if (position < endByte && document->source[position] != '\n') {
                return true;
            }
        }
        return false;
    }

}

void Highlighter::addImplicitMathNodes(WooWooDocument *document, std::vector<NodeInfo> &nodes) {

    DialectManager *dialectManager = DialectManager::getInstance();
    if (dialectManager->activeDialect == nullptr || !dialectManager->activeDialect->isImplicitOuterEnvironmentMath()) {
        return;
    }

    TSQueryCursor *mathCursor = ts_query_cursor_new();
    ts_query_cursor_exec(mathCursor, queries[implicitMathQuery], ts_tree_root_node(document->tree));

    TSQueryMatch match;
    while (ts_query_cursor_next_match(mathCursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; ++i) {
            TSNode capturedNode = match.captures[i].node;
            // an indented block inside a math environment is already coloured by the client
            if (isInMathEnvironment(document, capturedNode)) {
                continue;
            }
            addMathLineNodes(document, capturedNode, nodes);
        }
    }

    ts_query_cursor_delete(mathCursor);
}

void Highlighter::addMathLineNodes(WooWooDocument *document, const TSNode &node, std::vector<NodeInfo> &nodes) {

    // Semantic tokens cannot span several lines, so a multi-line block is processed line by
    // line: indentation and an optional meta block are skipped, the rest is LaTeX-tokenized.
    const std::string &source = document->source;
    const uint32_t nodeStart = ts_node_start_byte(node);
    const uint32_t nodeEnd = ts_node_end_byte(node);
    uint32_t line = ts_node_start_point(node).row;

    // the node starts at the beginning of a line, find the byte offset of that line start
    uint32_t lineStart = nodeStart;
    while (lineStart > 0 && source[lineStart - 1] != '\n') {
        --lineStart;
    }

    while (lineStart < nodeEnd && lineStart < source.size()) {

        uint32_t lineEnd = lineStart;
        while (lineEnd < source.size() && source[lineEnd] != '\n') {
            ++lineEnd;
        }

        uint32_t contentStart = std::max(lineStart, nodeStart);
        while (contentStart < lineEnd && (source[contentStart] == ' ' || source[contentStart] == '\t')) {
            ++contentStart;
        }

        uint32_t contentEnd = std::min(lineEnd, nodeEnd);
        while (contentEnd > contentStart &&
               (source[contentEnd - 1] == ' ' || source[contentEnd - 1] == '\t' || source[contentEnd - 1] == '\r')) {
            --contentEnd;
        }

        if (contentStart < contentEnd && !isLineInMetaBlock(document, line)) {
            addLatexLineNodes(document, line, lineStart, contentStart, contentEnd, nodes);
        }

        lineStart = lineEnd < source.size() ? lineEnd + 1 : source.size();
        ++line;
    }
}

void Highlighter::addLineToken(std::vector<NodeInfo> &nodes, uint32_t line, uint32_t lineStart,
                               uint32_t begin, uint32_t end, const std::string &tokenType) {
    if (begin >= end) {
        return;
    }
    TSPoint startPoint = {line, begin - lineStart};
    TSPoint endPoint = {line, end - lineStart};
    nodes.emplace_back(startPoint, endPoint, tokenType);
}

void Highlighter::addLatexLineNodes(WooWooDocument *document, uint32_t line, uint32_t lineStart,
                                    uint32_t contentStart, uint32_t contentEnd, std::vector<NodeInfo> &nodes) {

    // The TextMate grammar highlights the body of `$...$` and of `.equation:` with the `#latex`
    // rules, but it cannot recognise an anonymous (indented) block, so the same rules are
    // mirrored here with standard semantic token types (no client configuration needed):
    //   plain math -> number (the grammar uses `constant.numeric` for it)
    //   \command   -> function (grammar: support.function)
    //   \text{..}  -> function for the command, string for the content
    //   \begin{x}  -> function for the command, type for the environment name
    //   ^ _ = < > + - * / & | \\ -> operator (grammar: keyword.operator)
    //   % ...      -> comment (grammar: comment.line)
    const std::string &source = document->source;
    uint32_t baseStart = contentStart; // start of the pending plain math run
    uint32_t position = contentStart;

    // tokens of this line only, so that neighbouring ones of the same type are merged afterwards
    std::vector<NodeInfo> lineNodes;
    auto add = [&](uint32_t begin, uint32_t end, const std::string &tokenType) {
        addLineToken(lineNodes, line, lineStart, begin, end, tokenType);
    };
    auto flushBase = [&](uint32_t upTo) {
        add(baseStart, upTo, "number");
    };

    while (position < contentEnd) {
        char current = source[position];

        if (current == '%') {
            flushBase(position);
            add(position, contentEnd, "comment");
            position = contentEnd;
            baseStart = contentEnd;
            break;
        }

        if (current == '\\' && position + 1 < contentEnd) {
            char next = source[position + 1];

            if (next == '\\') { // line break
                flushBase(position);
                add(position, position + 2, "operator");
                position += 2;
                baseStart = position;
                continue;
            }

            uint32_t commandEnd = position + 1;
            while (commandEnd < contentEnd && isAsciiLetter(source[commandEnd])) {
                ++commandEnd;
            }
            std::string command = source.substr(position + 1, commandEnd - position - 1);

            if (command == "text") { // \text{...}, \text*{...}
                uint32_t brace = commandEnd;
                if (brace < contentEnd && source[brace] == '*') {
                    ++brace;
                }
                while (brace < contentEnd && (source[brace] == ' ' || source[brace] == '\t')) {
                    ++brace;
                }
                if (brace < contentEnd && source[brace] == '{') {
                    flushBase(position);
                    add(position, brace + 1, "function");
                    uint32_t textStart = brace + 1;
                    uint32_t textEnd = textStart;
                    while (textEnd < contentEnd && source[textEnd] != '}') {
                        ++textEnd;
                    }
                    add(textStart, textEnd, "string");
                    // the closing brace belongs to the command, as in the grammar's endCaptures
                    if (textEnd < contentEnd) {
                        add(textEnd, textEnd + 1, "function");
                    }
                    position = textEnd < contentEnd ? textEnd + 1 : contentEnd;
                    baseStart = position;
                    continue;
                }
            }

            if (command == "begin" || command == "end") { // \begin{env}, \end{env}
                uint32_t brace = commandEnd;
                while (brace < contentEnd && (source[brace] == ' ' || source[brace] == '\t')) {
                    ++brace;
                }
                if (brace < contentEnd && source[brace] == '{') {
                    uint32_t nameEnd = brace + 1;
                    while (nameEnd < contentEnd && source[nameEnd] != '}') {
                        ++nameEnd;
                    }
                    flushBase(position);
                    add(position, brace, "function");
                    // only the name inside the braces is a type in the grammar (capture 2 of
                    // `(\\\\(?:begin|end))\\s*\\{([^{}]*)\\}`), the braces stay plain math
                    add(brace, brace + 1, "number");
                    add(brace + 1, nameEnd, "type");
                    if (nameEnd < contentEnd) {
                        add(nameEnd, nameEnd + 1, "number");
                    }
                    position = nameEnd < contentEnd ? nameEnd + 1 : contentEnd;
                    baseStart = position;
                    continue;
                }
            }

            // any other command: \frac, \sin, \% ... (a single non-letter is a command too)
            uint32_t commandStop = commandEnd > position + 1 ? commandEnd : position + 2;
            flushBase(position);
            add(position, commandStop, "function");
            position = commandStop;
            baseStart = position;
            continue;
        }

        if (isLatexScriptChar(current) || isLatexOperatorChar(current)) {
            flushBase(position);
            add(position, position + 1, "operator");
            ++position;
            baseStart = position;
            continue;
        }

        ++position;
    }

    flushBase(contentEnd);

    // merge contiguous tokens of the same type (e.g. "2" + " " + "1" inside a plain run)
    for (const NodeInfo &lineNode: lineNodes) {
        if (!nodes.empty() && nodes.back().name == lineNode.name &&
            nodes.back().endPoint.row == lineNode.startPoint.row &&
            nodes.back().endPoint.column == lineNode.startPoint.column) {
            nodes.back().endPoint = lineNode.endPoint;
        } else {
            nodes.push_back(lineNode);
        }
    }
}


void Highlighter::setTokenTypes(std::vector<std::string> tokenTypesFromClient) {
    this->tokenTypes = std::move(tokenTypesFromClient);

    // build a map for fast access
    for (size_t i = 0; i < tokenTypes.size(); ++i) {
        tokenTypeIndices[tokenTypes[i]] = i;
    }
}

void Highlighter::setTokenModifiers(std::vector<std::string> tokenModifiersFromClient) {
    this->tokenModifiers = std::move(tokenModifiersFromClient);

    // build a map for fast access
    for (size_t i = 0; i < tokenModifiers.size(); ++i) {
        tokenModifierIndices[tokenModifiers[i]] = i;
    }
}


const std::unordered_map<std::string, std::pair<TSLanguage*, std::string>> &Highlighter::getQueryStringByName() const {
    return queryStringsByName;
}

const std::string Highlighter::woowooHighlightQuery = "woowooHighlightQuery";
const std::string Highlighter::yamlHighlightQuery = "yamlHighlightQuery";
const std::string Highlighter::implicitMathQuery = "implicitMathQuery";
const std::unordered_map<std::string, std::pair<TSLanguage*, std::string>> Highlighter::queryStringsByName = {
        {woowooHighlightQuery,
                std::make_pair(tree_sitter_woowoo(), R"(
; Include statement

(include) @keyword
(filename) @string


; Document part

;(document_part "." @operator)
;(document_part_type) @namespace

; Let client do the title, use LS just to highlight environments in title
;(document_part_title) @variable


; Wobject
;(wobject ["." ":"] @operator)
;(wobject_type) @storage.type.struct


; Block

;  - Short Inner Environment

;(short_inner_environment ["." ":"] @operator)
;(short_inner_environment_type) @type
;(short_inner_environment_body) @parameter

;  - Verbose Inner Environment

;(verbose_inner_environment (_ "\"" @string))
;(verbose_inner_environment (_ ["." "@" "#"] @operator))
;(verbose_inner_environment_type) @method
;(verbose_inner_environment_at_end) @method
;(verbose_inner_environment_meta) @modifier

;  - Outer Environment

;(outer_environment_type) @variable.other
;(fragile_outer_environment ["!" ":"] @operator)
;(classic_outer_environment ["." ":"] @operator)

;  - Math

;(math_environment "$" @function)
;(math_environment_body ) @number
)")},
        {implicitMathQuery, std::make_pair(tree_sitter_woowoo(), R"(
; Implicit (indented) outer environment, highlighted as math only if the dialect
; marks the implicit outer environment as math, see Dialect::isImplicitOuterEnvironmentMath

(implicit_outer_environment) @math
)")},
        {yamlHighlightQuery,std::make_pair(tree_sitter_yaml(), R"(
; Queries are from https://github.com/nvim-treesitter/nvim-treesitter/blob/master/queries/yaml/highlights.scm , but the order of queries was changed.
; The order of the query reflects the priority - if a given node is retrieved by multiple queries,
; the type that counts is the type given by the first query that retrieved the given node.

(block_mapping_pair
  key: (flow_node [(double_quote_scalar) (single_quote_scalar)] @property))
(block_mapping_pair
  key: (flow_node (plain_scalar (string_scalar) @property)))

(flow_mapping
  (_ key: (flow_node [(double_quote_scalar) (single_quote_scalar)] @property)))
(flow_mapping
  (_ key: (flow_node (plain_scalar (string_scalar) @property))))

(boolean_scalar) @keyword
(null_scalar) @enum
(double_quote_scalar) @string
(single_quote_scalar) @string
((block_scalar) @string (#set! "priority" 99))
(string_scalar) @string
(escape_sequence) @string
(integer_scalar) @number
(float_scalar) @number
(comment) @comment
(anchor_name) @type
(alias_name) @type
(tag) @type

[
  (yaml_directive)
  (tag_directive)
  (reserved_directive)
] @modifier

[
 ","
 "-"
 ":"
 ">"
 "?"
 "|"
] @operator

[
 "["
 "]"
 "{"
 "}"
] @operator

[
 "*"
 "&"
 "---"
 "..."
] @operator
)") }
};




