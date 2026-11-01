//
// Created by Michal Janecek on 08.08.2024.
//

#ifndef WUFF_WOOFILE_H
#define WUFF_WOOFILE_H

#include <filesystem>
#include <optional>
namespace fs = std::filesystem;

// The project file (YAML). Only the fields the analyzer uses are read; add new ones here.
class Woofile {
public:
    // Never throws: a missing or malformed Woofile gives a Woofile with no fields set (and a log line).
    static Woofile load(const fs::path &projectFolderPath);

    // builder.bibtex, absolute (a relative path is resolved against the folder of the Woofile)
    std::optional<fs::path> bibtex;
};


#endif //WUFF_WOOFILE_H
