//
// Created by Michal Janecek on 28.01.2024.
//

// UTF8toUTF16Mapping.cpp
#include "UTF8toUTF16Mapping.h"
void UTF8toUTF16Mapping::buildMappings(const std::string& source) {
    utf8ToUtf16Mappings.clear();
    utf16ToUtf8Mappings.clear();
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        addLineMappings(line);
    }
}

// Every byte of a character maps to the character's UTF-16 position, and every UTF-16 position maps
// back to the first byte of its character. The end of the line (a cursor after the last character)
// is mapped as well.
void UTF8toUTF16Mapping::addLineMappings(const std::string& line) {
    std::unordered_map<uint32_t, uint32_t> toUtf16;
    std::unordered_map<uint32_t, uint32_t> toUtf8;
    uint32_t utf8Offset = 0;
    uint32_t utf16Position = 0;

    while (utf8Offset < line.size()) {
        uint32_t charStart = utf8Offset;
        int charLen = utf8CharLen(line[utf8Offset]);
        if (charLen == 0 || charStart + charLen > line.size()) { // Error handling for invalid UTF-8
            std::cerr << "Invalid UTF-8 byte encountered" << std::endl;
            break;
        }

        uint32_t codePoint = utf8ToCodePoint(line, utf8Offset, charLen);
        int utf16Len = (codePoint >= 0x10000) ? 2 : 1;

        for (int i = 0; i < charLen; ++i) {
            toUtf16[charStart + i] = utf16Position;
        }
        for (int i = 0; i < utf16Len; ++i) {
            toUtf8[utf16Position + i] = charStart;
        }
        utf16Position += utf16Len;
    }
    toUtf16[utf8Offset] = utf16Position;
    toUtf8[utf16Position] = utf8Offset;

    utf8ToUtf16Mappings.push_back(std::move(toUtf16));
    utf16ToUtf8Mappings.push_back(std::move(toUtf8));
}


int UTF8toUTF16Mapping::utf8CharLen(unsigned char firstByte) {
    if (firstByte < 0x80) {
        return 1;
    } else if ((firstByte & 0xE0) == 0xC0) {
        return 2;
    } else if ((firstByte & 0xF0) == 0xE0) {
        return 3;
    } else if ((firstByte & 0xF8) == 0xF0) {
        return 4;
    }
    return 0; // Error case
}

uint32_t UTF8toUTF16Mapping::utf8ToCodePoint(const std::string& utf8, uint32_t & offset, int length) {
    uint32_t codePoint = 0;
    if (length == 1) {
        codePoint = utf8[offset];
    } else if (length == 2) {
        codePoint = ((utf8[offset] & 0x1F) << 6) | (utf8[offset + 1] & 0x3F);
    } else if (length == 3) {
        codePoint = ((utf8[offset] & 0x0F) << 12) | ((utf8[offset + 1] & 0x3F) << 6) | (utf8[offset + 2] & 0x3F);
    } else if (length == 4) {
        codePoint = ((utf8[offset] & 0x07) << 18) | ((utf8[offset + 1] & 0x3F) << 12) | ((utf8[offset + 2] & 0x3F) << 6) | (utf8[offset + 3] & 0x3F);
    }
    offset += length;
    return codePoint;
}


std::pair<uint32_t, uint32_t> UTF8toUTF16Mapping::utf8ToUtf16(uint32_t lineNum, uint32_t utf8Offset) const {
    if (lineNum < utf8ToUtf16Mappings.size()) {
        const auto& mapping = utf8ToUtf16Mappings[lineNum];
        auto it = mapping.find(utf8Offset);
        if (it != mapping.end()) {
            return {lineNum, it->second};
        }
    }
    // Return the original offset if the line number or utf8Offset is out of range or not found
    return {lineNum, utf8Offset};
}

std::pair<uint32_t, uint32_t> UTF8toUTF16Mapping::utf16ToUtf8(uint32_t lineNum, uint32_t utf16Offset) const {
    if (lineNum < utf16ToUtf8Mappings.size()) {
        const auto& mapping = utf16ToUtf8Mappings[lineNum];
        auto it = mapping.find(utf16Offset);
        if (it != mapping.end()) {
            return {lineNum, it->second};
        }
    }
    // Return the original offset if the line number or utf16Offset is out of range or not found
    return {lineNum, utf16Offset};
}

void UTF8toUTF16Mapping::utf8ToUtf16(Location & loc) const{
    utf8ToUtf16(loc.range);
}

void UTF8toUTF16Mapping::utf8ToUtf16(Range & range) const{
    auto startLine = range.start.line;
    auto startCol = range.start.character;
    auto endLine = range.end.line;
    auto endCol = range.end.character;

    auto transStart = utf8ToUtf16(startLine, startCol);
    auto transEnd = utf8ToUtf16(endLine, endCol);

    range.start.line = transStart.first;
    range.start.character = transStart.second;
    range.end.line = transEnd.first;
    range.end.character = transEnd.second;
}