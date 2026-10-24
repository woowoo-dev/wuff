//
// Created by Michal Janecek on 27.02.2024.
//

#ifndef WUFF_ENVIRONMENT_H
#define WUFF_ENVIRONMENT_H


#include <string>
#include <vector>
#include "Reference.h"
#include "MetaBlock.h"
#include "IDescribable.h"
#include "yaml-cpp/yaml.h"

class Environment : public IDescribable {
public:
    std::string name;
    std::string description;
    bool fragile;
    bool math = false;
    std::vector<Reference> references;
    MetaBlock metaBlock;
    Environment() = default;
    Environment(std::string  name, std::string  description, bool fragile = false, const std::vector<Reference>& references = {}, MetaBlock  metaBlock = MetaBlock());
    void deserialize(const YAML::Node& node);

    [[nodiscard]] std::string getDescription() const override {
        return description;
    }

    [[nodiscard]] std::string getName() const override {
        return name;
    }

};


#endif //WUFF_ENVIRONMENT_H
