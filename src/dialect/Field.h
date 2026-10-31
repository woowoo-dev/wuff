//
// Created by Michal Janecek on 27.01.2024.
//

#ifndef WUFF_FIELD_H
#define WUFF_FIELD_H


#include <string>
#include "yaml-cpp/yaml.h"

#include "Reference.h"


class Field {
public:
    std::string name;
    // Optional help text, shown in the hover of the construct that has this field.
    std::string description;
    std::vector<Reference> references; 

    explicit Field(std::string  name, const std::vector<Reference>& references);
    void deserialize(const YAML::Node& node);
    Field() = default;
};


#endif //WUFF_FIELD_H
