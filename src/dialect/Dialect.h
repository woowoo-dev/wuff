//
// Created by Michal Janecek on 27.01.2024.
//

#ifndef WUFF_DIALECT_H
#define WUFF_DIALECT_H


#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>
#include <memory>

#include "DocumentPart.h"
#include "Wobject.h"
#include "Environment.h"
#include "Shorthand.h"

class Dialect {
public:
    std::string name;
    std::string version_code;
    std::string version_name;
    std::string description;
    std::string implicit_outer_environment;

    std::vector<std::shared_ptr<DocumentPart>> document_parts;
    std::vector<std::shared_ptr<Wobject>> wobjects;
    std::vector<std::shared_ptr<Environment>> environments;

    std::shared_ptr<Shorthand> shorthand_hash;
    std::shared_ptr<Shorthand> shorthand_at;
    void deserialize(const YAML::Node& node);

    // Whether the environment with the given name is marked as math (see Environment::math).
    [[nodiscard]] bool isMathEnvironment(const std::string &environmentName) const;
    // Whether the dialect's implicit (indented) outer environment is a math one.
    [[nodiscard]] bool isImplicitOuterEnvironmentMath() const;
    
    
};



#endif //WUFF_DIALECT_H
