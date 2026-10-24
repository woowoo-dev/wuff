//
// Created by Michal Janecek on 27.01.2024.
//

#include "Field.h"

#include <utility>

Field::Field(std::string  name, const std::vector<Reference>& references) : name(std::move(name)), references(references) {
}

void Field::deserialize(const YAML::Node& node) {
    if (node["name"]) {
        name = node["name"].as<std::string>();
    } else {
        throw std::runtime_error("Field node does not have a 'name' attribute.");
    }

    if (node["description"]) {
        description = node["description"].as<std::string>();
    }

    // Deserialize 'references' if they exist
    if (node["references"]) {
        references.clear();  // Clear existing references before deserializing
        for (const auto& refNode : node["references"]) {
            Reference ref;
            ref.deserialize(refNode);
            references.push_back(ref);
        }
    }
    
}