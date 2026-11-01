//
// Created by Michal Janecek on 31.01.2024.
//

#ifndef WUFF_UTILS_H
#define WUFF_UTILS_H
class WooWooDocument;
#include <optional>
#include <string>
#include <tree_sitter/api.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace utils {

    std::string percentDecode(const std::string& encoded);
    std::string uriToPathString(const std::string& uri);
    // The path as a key of a document: lower case where file names are not case-sensitive (macOS, Windows),
    // like the paths uriToPathString gives, so a document is found whatever the case in its URI.
    std::string documentKey(const fs::path &documentPath);
    std::string pathToUri(const fs::path &documentPath);
    bool endsWith(const std::string &str, const std::string &suffix) ;
    std::optional<TSNode> getChild(TSNode node, const char *childTypes);
    std::string getChildText(TSNode node, const char *childType, WooWooDocument *doc);
    void appendToLogFile(const std::string & message);
    void reportQueryError(const std::string & queryName, uint32_t errorOffset, TSQueryError errorType);
} // namespace utils


#endif //WUFF_UTILS_H
