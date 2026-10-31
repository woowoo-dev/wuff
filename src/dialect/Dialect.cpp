//
// Created by Michal Janecek on 27.01.2024.
//

#include "Dialect.h"

void Dialect::deserialize(const YAML::Node& node) {
    if (!node["name"] || !node["version_code"] || !node["version_name"] || !node["description"] || !node["implicit_outer_environment"]) {
        throw std::runtime_error("Template YAML node is missing one or more required fields.");
    }

    name = node["name"].as<std::string>();
    version_code = node["version_code"].as<std::string>();
    version_name = node["version_name"].as<std::string>();
    description = node["description"].as<std::string>();
    implicit_outer_environment = node["implicit_outer_environment"].as<std::string>();

    // Deserialize DocumentParts
    if (node["document_parts"]) {
        for (const auto& dpNode : node["document_parts"]) {
            auto dp = std::make_shared<DocumentPart>();
            dp->deserialize(dpNode);
            document_parts.push_back(std::move(dp));
        }
    }

    // Deserialize Wobjects
    if (node["wobjects"]) {
        for (const auto& woNode : node["wobjects"]) {
            auto wo = std::make_shared<Wobject>();
            wo->deserialize(woNode);
            wobjects.push_back(std::move(wo));
        }
    }

    // Deserialize Environments
    if (node["environments"]) {
        for (const auto & envNode: node["environments"]) {
            auto environment = std::make_shared<Environment>();
            environment->deserialize(envNode);
            environments.push_back(std::move(environment));
        }
    }
    
    // Deserialize Shorthands
    if (node["shorthands"]["hash"]) {
        shorthand_hash = std::make_shared<Shorthand>();
        shorthand_hash->deserialize(node["shorthands"]["hash"]);
        shorthand_hash->type = "hash";
    }

    if (node["shorthands"]["at"]) {
        shorthand_at = std::make_shared<Shorthand>();
        shorthand_at->deserialize(node["shorthands"]["at"]);
        shorthand_at->type = "at";
    }
}

bool Dialect::isMathEnvironment(const std::string &environmentName) const {
    for (const auto &environment: environments) {
        if (environment->name == environmentName) {
            return environment->math;
        }
    }
    return false;
}

bool Dialect::isImplicitOuterEnvironmentMath() const {
    return isMathEnvironment(implicit_outer_environment);
}