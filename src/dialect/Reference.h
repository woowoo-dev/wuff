//
// Created by Michal Janecek on 27.01.2024.
//

#ifndef WUFF_REFERENCE_H
#define WUFF_REFERENCE_H

#include <string>
#include <functional>  // For std::hash
#include "yaml-cpp/yaml.h"


// What the value of a referencing construct points at.
enum class ReferenceTarget {
    MetaField,    // a meta-block field value somewhere in the project (e.g. `label: x`)
    Bibliography  // an entry key in the project's bibliography (Woofile: builder.bibtex)
};

class Reference {
public:
    ReferenceTarget target = ReferenceTarget::MetaField;
    std::string metaKey;
    std::string structureType;
    std::string structureName;

    Reference() = default;
    Reference(const std::string& metaKey, const std::string& structureType = "", const std::string& structureName = "");
    void deserialize(const YAML::Node& node);

    // True when the target lists every valid value, so a value that is not among them is a mistake.
    // Labels are not complete: a label can be defined in another project (the Woofile catalogue).
    [[nodiscard]] bool isComplete() const;
    // True when both references point at the same set of values.
    [[nodiscard]] bool sharesTargetWith(const Reference& other) const;

    bool operator==(const Reference& other) const;
};

namespace std {
    template <>
    struct hash<Reference> {
        std::size_t operator()(const Reference& ref) const noexcept;
    };
}

#endif 
