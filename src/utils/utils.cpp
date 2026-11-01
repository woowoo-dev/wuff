//
// Created by Michal Janecek on 31.01.2024.
//

#include <algorithm>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <tree_sitter/api.h>

#include "../project/WooWooDocument.h"


namespace utils {
    namespace {
        // ASCII letters only, byte by byte: the bytes of UTF-8 sequences stay as they are
        std::string toLowerAscii(std::string text) {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
                return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : static_cast<char>(c);
            });
            return text;
        }
    }

    std::string percentDecode(const std::string &encoded) {
        std::string decoded;
        for (size_t i = 0; i < encoded.length(); ++i) {
            if (encoded[i] == '%' && i + 2 < encoded.length()) {
                std::string hex = encoded.substr(i + 1, 2);
                char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
                decoded += ch;
                i += 2; // Skip the next two characters
            } else {
                decoded += encoded[i];
            }
        }
        return decoded;
    }

    std::string uriToPathString(const std::string &uri) {
        // Assuming the URI starts with 'file://'
        if (uri.substr(0, 7) != "file://") {
            throw std::invalid_argument("URI does not start with 'file://'");
        }

        std::string path = uri.substr(7); // Remove 'file://'

        path = percentDecode(path); // Decode any percent-encoded characters

#ifdef __APPLE__
        path = toLowerAscii(path);
#endif


#ifdef _WIN32
        path = toLowerAscii(path);
        // Windows file URIs start with a '/', which should not be present in the final path
        // Additionally, we need to handle drive letters (e.g., 'C:/')
        if (path.size() > 1 && path[0] == '/' && path[2] == ':') {
            path.erase(0, 1); // Remove the leading '/'
        }
#else
        // On Unix-like systems, no additional processing is needed
#endif

        return fs::path(path).generic_string(); // Convert to generic path format
    }


    std::string documentKey(const fs::path &documentPath) {
#if defined(__APPLE__) || defined(_WIN32)
        return toLowerAscii(documentPath.generic_string());
#else
        return documentPath.generic_string();
#endif
    }


    std::string pathToUri(const fs::path &documentPath) {
        std::string uri = "file://";

        // On Windows, prepend with '/' to accommodate the drive letter and colon
#ifdef _WIN32
        if (!documentPath.empty()) {
            uri += '/';
            // Use the generic string and replace backslashes with forward slashes
            std::string pathStr = documentPath.generic_string();
            uri += pathStr;
        }
#else
        uri += documentPath.generic_string();
#endif

        return uri;
    }

    std::optional<TSNode> getChild(TSNode node, const char *childType) {
        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; ++i) {
            TSNode child = ts_node_child(node, i);
            if (strcmp(ts_node_type(child), childType) == 0) {
                return child;
            }
        }
        return std::nullopt;
    }

    bool endsWith(const std::string &str, const std::string &suffix) {
        if (str.length() >= suffix.length()) {
            return (str.rfind(suffix) == (str.length() - suffix.length()));
        } else {
            return false;
        }
    }

    std::string getChildText(TSNode node, const char *childType, WooWooDocument *doc) {
        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; ++i) {
            TSNode child = ts_node_child(node, i);
            if (strcmp(ts_node_type(child), childType) == 0) {
                return doc->getNodeText(child);
            }
        }
        return ""; // Return an empty string if no matching child is found
    }

    void reportQueryError(const std::string &queryName, uint32_t errorOffset, TSQueryError errorType) {
        std::string errorMessage = "Error compiling query '" + queryName + "': ";

        switch (errorType) {
            case TSQueryErrorSyntax:
                errorMessage += "Syntax error";
                break;
            case TSQueryErrorNodeType:
                errorMessage += "Invalid node type";
                break;
            case TSQueryErrorField:
                errorMessage += "Invalid field name";
                break;
            case TSQueryErrorCapture:
                errorMessage += "Invalid capture name";
                break;
            default:
                errorMessage += "Unknown error";
                break;
        }

        errorMessage += " at offset " + std::to_string(errorOffset) + ".";

        throw std::runtime_error(errorMessage);
    }

    void appendToLogFile(const std::string &message) {
        std::ofstream logFile("log.txt", std::ios::app);

        if (!logFile) {
            std::cerr << "Failed to open log.txt for appending." << std::endl;
            return;
        }

        // Append the message to the file with a newline
        logFile << message << std::endl;

        logFile.close();
    }


} // namespace utils
