//
// Created by Michal Janecek on 08.08.2024.
//

#include "Woofile.h"

Woofile::Woofile(const fs::path &projectFolderPath) {
    YAML::Node yamlData = YAML::LoadFile((projectFolderPath / "Woofile").string());
    deserialize(yamlData);
    // TODO: if bibtex is set, load it and process it
    // https://gitlab.fit.cvut.cz/BI-MA1/bi-ma1/-/blob/master/src/textbook/references.bib
}

void Woofile::deserialize(const YAML::Node &node) {
    if (node["builder"] && node["builder"]["bibtex"]) {
        bibtex = node["builder"]["bibtex"].as<std::string>();
    } else {
        bibtex.clear();
    }
}

