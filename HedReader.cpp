#include "HedReader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>

// ------------------------------------------------------------
// HedReader::Load
//
// KH 1.5/2.5 HD EGS HED format:
//
// 0x00 - 0x0F : MD5          (16 bytes)
// 0x10 - 0x17 : Offset        (8 bytes)
// 0x18 - 0x1B : DataLength    (4 bytes)
// 0x1C - 0x1F : ActualLength  (4 bytes)
//
// Entry size = 0x20 (32 bytes)
// ------------------------------------------------------------

bool HedReader::Load(
    const std::string& path)
{
    entries.clear();

    std::cout
        << "[KH] HedReader::Load()"
        << std::endl;

    std::cout
        << "[KH] File: "
        << path
        << std::endl;

    std::ifstream file(
        path,
        std::ios::binary);

    if (!file.is_open())
    {
        std::cout
            << "[KH] Could not open HED file."
            << std::endl;

        return false;
    }

    // --------------------------------------------------------
    // Get file size
    // --------------------------------------------------------

    file.seekg(
        0,
        std::ios::end);

    const std::streamoff fileSize =
        file.tellg();

    file.seekg(
        0,
        std::ios::beg);

    if (fileSize <= 0)
    {
        std::cout
            << "[KH] HED file is empty."
            << std::endl;

        return false;
    }

    constexpr std::streamoff ENTRY_SIZE = 0x20;

    // --------------------------------------------------------
    // Validate HED size
    // --------------------------------------------------------

    if ((fileSize % ENTRY_SIZE) != 0)
    {
        std::cout
            << "[KH] Invalid HED size: "
            << fileSize
            << std::endl;

        std::cout
            << "[KH] Expected a multiple of 0x20."
            << std::endl;

        return false;
    }

    const size_t entryCount =
        static_cast<size_t>(
            fileSize / ENTRY_SIZE);

    std::cout
        << "[KH] HED size: "
        << fileSize
        << " bytes"
        << std::endl;

    std::cout
        << "[KH] Entry size: "
        << ENTRY_SIZE
        << " bytes"
        << std::endl;

    std::cout
        << "[KH] Entry count: "
        << entryCount
        << std::endl;

    // --------------------------------------------------------
    // Reserve entries
    // --------------------------------------------------------

    entries.reserve(entryCount);

    // --------------------------------------------------------
    // Read entries
    // --------------------------------------------------------

    for (size_t i = 0;
        i < entryCount;
        ++i)
    {
        Entry entry;

        // ----------------------------------------------------
        // MD5
        // ----------------------------------------------------

        file.read(
            reinterpret_cast<char*>(
                entry.md5.data()),
            static_cast<std::streamsize>(
                entry.md5.size()));

        if (!file)
        {
            std::cout
                << "[KH] Failed reading MD5 for entry "
                << i
                << std::endl;

            entries.clear();

            return false;
        }

        // ----------------------------------------------------
        // Offset
        //
        // 8-byte little-endian signed integer
        // ----------------------------------------------------

        file.read(
            reinterpret_cast<char*>(
                &entry.offset),
            sizeof(entry.offset));

        if (!file)
        {
            std::cout
                << "[KH] Failed reading offset for entry "
                << i
                << std::endl;

            entries.clear();

            return false;
        }

        // ----------------------------------------------------
        // Data length
        // ----------------------------------------------------

        file.read(
            reinterpret_cast<char*>(
                &entry.dataLength),
            sizeof(entry.dataLength));

        if (!file)
        {
            std::cout
                << "[KH] Failed reading data length for entry "
                << i
                << std::endl;

            entries.clear();

            return false;
        }

        // ----------------------------------------------------
        // Actual length
        // ----------------------------------------------------

        file.read(
            reinterpret_cast<char*>(
                &entry.actualLength),
            sizeof(entry.actualLength));

        if (!file)
        {
            std::cout
                << "[KH] Failed reading actual length for entry "
                << i
                << std::endl;

            entries.clear();

            return false;
        }

        entries.push_back(entry);
    }

    // --------------------------------------------------------
    // Success
    // --------------------------------------------------------

    std::cout
        << "[KH] Successfully loaded "
        << entries.size()
        << " HED entries."
        << std::endl;

    // --------------------------------------------------------
    // Print first few entries for verification
    // --------------------------------------------------------

    const size_t previewCount =
        std::min<size_t>(
            entries.size(),
            3);

    for (size_t i = 0;
        i < previewCount;
        ++i)
    {
        const Entry& entry =
            entries[i];

        std::cout
            << "[KH] Entry "
            << i
            << ": offset="
            << entry.offset
            << ", dataLength="
            << entry.dataLength
            << ", actualLength="
            << entry.actualLength
            << std::endl;
    }

    return !entries.empty();
}

// ------------------------------------------------------------
// Get entries
// ------------------------------------------------------------

const std::vector<HedReader::Entry>&
HedReader::GetEntries() const
{
    return entries;
}