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
    std::string pathToUri(const fs::path &documentPath);
    bool endsWith(const std::string &str, const std::string &suffix) ;
    std::optional<TSNode> getChild(TSNode node, const char *childTypes);
    std::string getChildText(TSNode node, const char *childType, WooWooDocument *doc);
    void appendToLogFile(const std::string & message);
    void reportQueryError(const std::string & queryName, uint32_t errorOffset, TSQueryError errorType);
} // namespace utils


#endif //WUFF_UTILS_H
