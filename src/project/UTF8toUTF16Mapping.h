//
// Created by Michal Janecek on 28.01.2024.
//

#ifndef WUFF_UTF8TOUTF16MAPPING_H
#define WUFF_UTF8TOUTF16MAPPING_H


#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "../lsp/LSPTypes.h"

class UTF8toUTF16Mapping {
public:
    void buildMappings(const std::string& source);

    [[nodiscard]] std::pair<uint32_t, uint32_t> utf8ToUtf16(uint32_t lineNum, uint32_t utf8Offset) const;

    [[nodiscard]] std::pair<uint32_t, uint32_t> utf16ToUtf8(uint32_t lineNum, uint32_t utf16Offset) const;

    void utf8ToUtf16(Location & loc) const;
    void utf8ToUtf16(Range & r) const;

private:
    std::vector<std::unordered_map<uint32_t, uint32_t>> utf8ToUtf16Mappings;
    std::vector<std::unordered_map<uint32_t, uint32_t>> utf16ToUtf8Mappings;
    void addLineMappings(const std::string& line);
    static int utf8CharLen(unsigned char firstByte);
    static uint32_t utf8ToCodePoint(const std::string& utf8, uint32_t & offset, int length);
};

#endif //WUFF_UTF8TOUTF16MAPPING_H
