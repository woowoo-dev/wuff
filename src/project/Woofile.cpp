//
// Created by Michal Janecek on 08.08.2024.
//

#include "Woofile.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

Woofile Woofile::load(const fs::path &projectFolderPath) {
    Woofile woofile;
    fs::path woofilePath = projectFolderPath / "Woofile";
    try {
        // a missing key gives an undefined node, which is false; asking an undefined node for its type throws
        const YAML::Node root = YAML::LoadFile(woofilePath.string());
        const YAML::Node builder = root.IsMap() ? root["builder"] : YAML::Node();
        const YAML::Node bibtexNode = builder && builder.IsMap() ? builder["bibtex"] : YAML::Node();
        if (bibtexNode && bibtexNode.IsScalar()) {
            fs::path bibtex = bibtexNode.as<std::string>();
            if (!bibtex.empty()) {
                woofile.bibtex = (projectFolderPath / bibtex).lexically_normal();  // an absolute bibtex wins
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Could not read " << woofilePath.string() << ": " << e.what() << std::endl;
    }
    return woofile;
}
