#include "Bibliography.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include "../parser/Parser.h"
#include "UTF8toUTF16Mapping.h"
#include <iostream>

namespace {

// Every byte that is not part of valid UTF-8 becomes U+FFFD (as in the editor's view of the file), so the
// entries can be passed to Python and their positions match the editor's.
std::string withValidUtf8(const std::string &text) {
    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        auto byte = [&](size_t at) { return at < text.size() ? static_cast<unsigned char>(text[at]) : 0u; };
        unsigned char lead = byte(i);
        size_t length = lead < 0x80 ? 1 : lead >= 0xC2 && lead <= 0xDF ? 2 : lead >= 0xE0 && lead <= 0xEF ? 3
                                                                    : lead >= 0xF0 && lead <= 0xF4 ? 4 : 0;
        // the second byte is limited for E0 (overlong), ED (surrogates), F0 (overlong) and F4 (above U+10FFFF)
        unsigned char low = lead == 0xE0 ? 0xA0 : lead == 0xF0 ? 0x90 : 0x80;
        unsigned char high = lead == 0xED ? 0x9F : lead == 0xF4 ? 0x8F : 0xBF;
        bool valid = length > 0;
        for (size_t k = 1; valid && k < length; ++k) {
            unsigned char next = byte(i + k);
            valid = k == 1 ? next >= low && next <= high : next >= 0x80 && next <= 0xBF;
        }
        if (valid) {
            result.append(text, i, length);
            i += length;
        } else {
            result += "\xEF\xBF\xBD";
            i += 1;
        }
    }
    return result;
}

std::string nodeText(TSNode node, const std::string &source) {
    return source.substr(ts_node_start_byte(node), ts_node_end_byte(node) - ts_node_start_byte(node));
}

// ASCII only: the <cctype> functions follow the locale, and in a UTF-8 locale they take single bytes of UTF-8
// sequences for Latin-1 characters (A0, the second byte of `Š`, would be a space)
bool isAsciiSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    });
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
        if (isAsciiSpace(c)) {
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
        std::cerr << "Could not read the bibliography " << bibFile.string() << std::endl;
        return nullptr;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::unique_ptr<Bibliography> bibliography(new Bibliography(bibFile, withValidUtf8(buffer.str())));
    if (bibliography->entries().empty()) {
        // a folder, a file in another encoding (UTF-16), or no entries at all: nothing to check citations against
        std::cerr << "No bibliography entries in " << bibFile.string() << std::endl;
        return nullptr;
    }
    return bibliography;
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
