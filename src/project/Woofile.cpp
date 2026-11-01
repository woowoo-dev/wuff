//
// Created by Michal Janecek on 08.08.2024.
//

#include "Woofile.h"
#include "yaml-cpp/yaml.h"
#include "../utils/utils.h"

Woofile Woofile::load(const fs::path &projectFolderPath) {
    Woofile woofile;
    fs::path woofilePath = projectFolderPath / "Woofile";
    try {
        const YAML::Node root = YAML::LoadFile(woofilePath.string());
        if (root.IsMap() && root["builder"].IsMap() && root["builder"]["bibtex"].IsScalar()) {
            fs::path bibtex = root["builder"]["bibtex"].as<std::string>();
            if (!bibtex.empty()) {
                woofile.bibtex = (projectFolderPath / bibtex).lexically_normal();  // an absolute bibtex wins
            }
        }
    } catch (const std::exception &e) {
        utils::appendToLogFile("Could not read " + woofilePath.string() + ": " + e.what());
    }
    return woofile;
}
