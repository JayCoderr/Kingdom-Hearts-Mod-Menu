#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class HedReader
{
public:

    struct Entry
    {
        // 0x00 - 0x0F
        std::array<uint8_t, 16> md5{};

        // 0x10 - 0x17
        int64_t offset = 0;

        // 0x18 - 0x1B
        int32_t dataLength = 0;

        // 0x1C - 0x1F
        int32_t actualLength = 0;
    };

    bool Load(
        const std::string& path);

    const std::vector<Entry>&
        GetEntries() const;

private:

    std::vector<Entry> entries;
};