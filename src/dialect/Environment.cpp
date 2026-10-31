//
// Created by Michal Janecek on 27.02.2024.
//

#include "Environment.h"


Environment::Environment(std::string  name, std::string  description, bool fragile, const std::vector<Reference>& references, MetaBlock  metaBlock)
        : name(std::move(name)), description(std::move(description)), fragile(fragile), references(references), metaBlock(std::move(metaBlock)) {
}

void Environment::deserialize(const YAML::Node& node) {
    if (!node["name"] || !node["description"]) {
        throw std::runtime_error("InnerEnvironment YAML node is missing 'name' or 'description'");
    }

    name = node["name"].as<std::string>();
    description = node["description"].as<std::string>();
    fragile = node["fragile"].as<bool>(false);
    // Marks environments whose body is math (e.g. equation/align/gather). The highlighter uses it
    // to decide whether the implicit (indented) outer environment is math, and to leave a block
    // nested in a math environment body to the client's own math highlighting.
    math = node["math"].as<bool>(false);
    
    // Deserialize 'references' if they exist
    if (node["references"]) {
        references.clear();  // Clear existing references before deserializing
        for (const auto& refNode : node["references"]) {
            Reference ref;
            ref.deserialize(refNode);
            references.push_back(ref);
        }
    }

    // Deserialize 'metaBlock' if it exists
    if (node["meta_block"]) {
        metaBlock.deserialize(node["meta_block"]);
    }
}