#include "Bibliography.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include "../parser/Parser.h"
#include "UTF8toUTF16Mapping.h"
#include "../utils/utils.h"

namespace {

std::string nodeText(TSNode node, const std::string &source) {
    return source.substr(ts_node_start_byte(node), ts_node_end_byte(node) - ts_node_start_byte(node));
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

std::string trim(const std::string &text) {
    auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

// `{Numerical {L}inear\n   Algebra}` -> `Numerical Linear Algebra`; the pieces of `a # "b"` are joined
std::string fieldValue(TSNode valueNode, const std::string &source) {
    std::string joined;
    for (uint32_t i = 0; i < ts_node_named_child_count(valueNode); ++i) {
        std::string piece = nodeText(ts_node_named_child(valueNode, i), source);
        if (piece.size() >= 2 && ((piece.front() == '{' && piece.back() == '}') ||
                                  (piece.front() == '"' && piece.back() == '"'))) {
            piece = piece.substr(1, piece.size() - 2);
        }
        joined += piece;
    }

    std::string value;
    for (char c: joined) {
        if (c == '{' || c == '}') continue;
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!value.empty() && value.back() != ' ') value += ' ';
        } else {
            value += c;
        }
    }
    return trim(value);
}

// "Trefethen, Lloyd N. and Bau, David" -> "Trefethen, Bau"
std::string lastNames(const std::string &authors) {
    std::string result;
    std::string rest = authors;
    while (!rest.empty()) {
        auto separator = rest.find(" and ");
        std::string name = trim(rest.substr(0, separator));
        rest = separator == std::string::npos ? "" : rest.substr(separator + 5);

        auto comma = name.find(',');
        std::string last = comma != std::string::npos ? trim(name.substr(0, comma))
                                                      : name.substr(name.find_last_of(' ') + 1);
        if (last.empty()) continue;
        if (!result.empty()) result += ", ";
        result += last;
    }
    return result;
}

} // namespace

std::string BibliographyEntry::summary() const {
    std::string result = lastNames(author);
    if (!year.empty()) {
        result += (result.empty() ? "(" : " (") + year + ")";
    }
    if (!title.empty()) {
        result += (result.empty() ? "" : ": ") + title;
    }
    return result;
}

std::string BibliographyEntry::markdown() const {
    std::string details;
    for (const std::string *part: {&author, &year, &type}) {
        if (part->empty()) continue;
        if (!details.empty()) details += " · ";
        details += *part;
    }
    if (title.empty()) return details;
    return details.empty() ? "**" + title + "**" : "**" + title + "**\n\n" + details;
}

std::unique_ptr<Bibliography> Bibliography::load(const fs::path &bibFile) {
    std::ifstream file(bibFile, std::ios::in | std::ios::binary);
    if (!file) {
        utils::appendToLogFile("Could not read the bibliography " + bibFile.string());
        return nullptr;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return std::unique_ptr<Bibliography>(new Bibliography(bibFile, buffer.str()));
}

Bibliography::Bibliography(fs::path filePath, const std::string &source) : filePath(std::move(filePath)) {
    UTF8toUTF16Mapping utfMapping;
    utfMapping.buildMappings(source);

    TSTree *tree = Parser::getInstance()->parseBibTeX(source);
    TSNode root = ts_tree_root_node(tree);
    for (uint32_t i = 0; i < ts_node_named_child_count(root); ++i) {
        TSNode entryNode = ts_node_named_child(root, i);
        if (std::string(ts_node_type(entryNode)) != "entry") continue;  // @comment, @string, @preamble, junk

        TSNode keyNode = ts_node_child_by_field_name(entryNode, "key", 3);
        if (ts_node_is_null(keyNode) || ts_node_is_missing(keyNode)) continue;
        BibliographyEntry entry;
        entry.key = nodeText(keyNode, source);
        if (entry.key.empty() || entryIndexByKey.contains(entry.key)) continue;  // the first entry of a key wins

        TSNode typeNode = ts_node_child_by_field_name(entryNode, "ty", 2);
        if (!ts_node_is_null(typeNode)) {
            entry.type = toLower(nodeText(typeNode, source));
            if (!entry.type.empty() && entry.type.front() == '@') entry.type.erase(0, 1);
        }

        for (uint32_t j = 0; j < ts_node_named_child_count(entryNode); ++j) {
            TSNode fieldNode = ts_node_named_child(entryNode, j);
            if (std::string(ts_node_type(fieldNode)) != "field") continue;
            TSNode nameNode = ts_node_child_by_field_name(fieldNode, "name", 4);
            TSNode valueNode = ts_node_child_by_field_name(fieldNode, "value", 5);
            if (ts_node_is_null(nameNode) || ts_node_is_null(valueNode)) continue;

            auto name = toLower(nodeText(nameNode, source));
            if (name == "author") entry.author = fieldValue(valueNode, source);
            else if (name == "title") entry.title = fieldValue(valueNode, source);
            else if (name == "year") entry.year = fieldValue(valueNode, source);
        }

        TSPoint s = ts_node_start_point(keyNode);
        TSPoint e = ts_node_end_point(keyNode);
        entry.keyRange = Range{{s.row, s.column}, {e.row, e.column}};
        utfMapping.utf8ToUtf16(entry.keyRange);

        entryIndexByKey[entry.key] = entryList.size();
        entryList.push_back(std::move(entry));
    }
    ts_tree_delete(tree);
}

const BibliographyEntry *Bibliography::find(const std::string &key) const {
    auto it = entryIndexByKey.find(key);
    return it == entryIndexByKey.end() ? nullptr : &entryList[it->second];
}

const std::vector<BibliographyEntry> &Bibliography::entries() const {
    return entryList;
}

const fs::path &Bibliography::path() const {
    return filePath;
}
