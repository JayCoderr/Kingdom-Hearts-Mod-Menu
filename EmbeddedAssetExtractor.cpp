#include "EmbeddedAssetExtractor.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>

#include "miniz.h"

// ============================================================
// Basic readers
// ============================================================

static uint16_t ReadU16LE(const uint8_t* data)
{
    return static_cast<uint16_t>(
        static_cast<uint16_t>(data[0]) |
        (static_cast<uint16_t>(data[1]) << 8));
}

static int32_t ReadI32LE(const uint8_t* data)
{
    return static_cast<int32_t>(
        static_cast<uint32_t>(data[0]) |
        (static_cast<uint32_t>(data[1]) << 8) |
        (static_cast<uint32_t>(data[2]) << 16) |
        (static_cast<uint32_t>(data[3]) << 24));
}

static uint32_t ReadU32LE(const uint8_t* data)
{
    return
        static_cast<uint32_t>(data[0]) |
        (static_cast<uint32_t>(data[1]) << 8) |
        (static_cast<uint32_t>(data[2]) << 16) |
        (static_cast<uint32_t>(data[3]) << 24);
}

// ============================================================
// Safe range check
// ============================================================

static bool IsValidRange(
    size_t offset,
    size_t size,
    size_t total)
{
    if (offset > total)
        return false;

    if (size > total - offset)
        return false;

    return true;
}

// ============================================================
// Safe signed range check
// ============================================================

static bool IsValidSignedRange(
    int32_t offset,
    int32_t size,
    size_t total)
{
    if (offset < 0 || size < 0)
        return false;

    return IsValidRange(
        static_cast<size_t>(offset),
        static_cast<size_t>(size),
        total);
}

// ============================================================
// Safe align to 0x10
// ============================================================

static bool Align16Checked(
    size_t value,
    size_t& aligned)
{
    if (value >
        std::numeric_limits<size_t>::max() - 0x0F)
    {
        aligned = 0;
        return false;
    }

    aligned =
        (value + 0x0F) &
        ~static_cast<size_t>(0x0F);

    return true;
}

// ============================================================
// FourCC
// ============================================================

static constexpr uint32_t MakeFourCC(
    char a,
    char b,
    char c,
    char d)
{
    return
        static_cast<uint32_t>(
            static_cast<uint8_t>(a)) |
        (static_cast<uint32_t>(
            static_cast<uint8_t>(b)) << 8) |
        (static_cast<uint32_t>(
            static_cast<uint8_t>(c)) << 16) |
        (static_cast<uint32_t>(
            static_cast<uint8_t>(d)) << 24);
}

static bool IsFourCC(
    const std::vector<uint8_t>& data,
    size_t offset,
    uint32_t fourCC)
{
    if (!IsValidRange(offset, 4, data.size()))
        return false;

    return ReadU32LE(
        data.data() + offset) == fourCC;
}

// ============================================================
// Hex dump
// ============================================================

static void DumpHex(
    const std::vector<uint8_t>& data,
    size_t start,
    size_t length)
{
    if (start >= data.size())
        return;

    const size_t maxLength =
        std::min(
            length,
            data.size() - start);

    const size_t end =
        start + maxLength;

    for (size_t i = start; i < end; i += 16)
    {
        std::cout
            << "[KH-DUMP]   "
            << std::hex
            << std::setw(8)
            << std::setfill('0')
            << i
            << "  ";

        for (size_t j = 0; j < 16; ++j)
        {
            const size_t index = i + j;

            if (index < end)
            {
                std::cout
                    << std::setw(2)
                    << static_cast<unsigned int>(
                        data[index])
                    << " ";
            }
            else
            {
                std::cout << "   ";
            }
        }

        std::cout << " | ";

        for (size_t j = 0; j < 16; ++j)
        {
            const size_t index = i + j;

            if (index >= end)
                break;

            const uint8_t c = data[index];

            if (c >= 32 && c <= 126)
                std::cout << static_cast<char>(c);
            else
                std::cout << '.';
        }

        std::cout
            << std::dec
            << std::setfill('0')
            << std::endl;
    }
}

// ============================================================
// ASCII reader
// ============================================================

static std::string ReadAsciiString(
    const std::vector<uint8_t>& data,
    size_t offset)
{
    if (offset >= data.size())
        return {};

    std::string result;

    for (size_t i = offset;
        i < data.size();
        ++i)
    {
        const uint8_t c = data[i];

        if (c == 0)
            break;

        if (c < 32 || c > 126)
            break;

        result.push_back(
            static_cast<char>(c));

        if (result.size() >= 256)
            break;
    }

    return result;
}

// ============================================================
// Resource extension
// ============================================================

static bool HasResourceExtension(
    const std::string& name)
{
    std::string lower = name;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c));
        });

    const char* extensions[] =
    {
        ".dds",
        ".cvbl",
        ".gvbl",
        ".mvbl",
        ".wpn",
        ".mdls"
    };

    for (const char* extension : extensions)
    {
        const size_t length =
            std::strlen(extension);

        if (lower.size() >= length &&
            lower.compare(
                lower.size() - length,
                length,
                extension) == 0)
        {
            return true;
        }
    }

    return false;
}

// ============================================================
// Sanitize filename
// ============================================================

static std::string SanitizeFilename(
    const std::string& input)
{
    std::string result = input;

    for (char& c : result)
    {
        switch (c)
        {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            c = '_';
            break;

        default:
            break;
        }
    }

    if (result.empty())
        result = "resource";

    return result;
}

// ============================================================
// Make unique output path
// ============================================================

static std::filesystem::path MakeUniqueOutputPath(
    const std::filesystem::path& directory,
    const std::string& filename)
{
    std::filesystem::path result =
        directory / filename;

    if (!std::filesystem::exists(result))
        return result;

    const std::filesystem::path original =
        result;

    const std::string stem =
        original.stem().string();

    const std::string extension =
        original.extension().string();

    for (unsigned int i = 1; i < 100000; ++i)
    {
        result =
            directory /
            (
                stem +
                "_" +
                std::to_string(i) +
                extension
                );

        if (!std::filesystem::exists(result))
            return result;
    }

    return
        directory /
        (
            stem +
            "_unique" +
            extension
            );
}

// ============================================================
// Write bytes
// ============================================================

static bool WriteBytes(
    const std::vector<uint8_t>& data,
    size_t offset,
    size_t size,
    const std::filesystem::path& path)
{
    if (!IsValidRange(
        offset,
        size,
        data.size()))
    {
        return false;
    }

    if (size == 0)
        return false;

    try
    {
        const auto parent =
            path.parent_path();

        if (!parent.empty())
        {
            std::filesystem::create_directories(
                parent);
        }
    }
    catch (...)
    {
        return false;
    }

    std::ofstream file(
        path,
        std::ios::binary);

    if (!file.is_open())
        return false;

    file.write(
        reinterpret_cast<const char*>(
            data.data() + offset),
        static_cast<std::streamsize>(
            size));

    return file.good();
}

// ============================================================
// DDS detection
// ============================================================

static bool IsDDS(
    const std::vector<uint8_t>& data,
    size_t offset)
{
    return IsFourCC(
        data,
        offset,
        MakeFourCC('D', 'D', 'S', ' '));
}

// ============================================================
// Find literal DDS headers
// ============================================================

static std::vector<size_t> FindDDSHeaders(
    const std::vector<uint8_t>& data)
{
    std::vector<size_t> result;

    for (size_t i = 0;
        i + 4 <= data.size();
        ++i)
    {
        if (IsDDS(data, i))
            result.push_back(i);
    }

    return result;
}

// ============================================================
// Calculate DDS size
// ============================================================

static bool CalculateDDSSize(
    const std::vector<uint8_t>& data,
    size_t offset,
    size_t& size)
{
    size = 0;

    if (!IsValidRange(
        offset,
        128,
        data.size()))
    {
        return false;
    }

    if (!IsDDS(
        data,
        offset))
    {
        return false;
    }

    if (ReadU32LE(
        data.data() + offset + 4) != 124)
    {
        return false;
    }

    const uint32_t height =
        ReadU32LE(
            data.data() + offset + 12);

    const uint32_t width =
        ReadU32LE(
            data.data() + offset + 16);

    if (width == 0 ||
        height == 0 ||
        width > 16384 ||
        height > 16384)
    {
        return false;
    }

    size_t next =
        data.size();

    for (size_t i = offset + 128;
        i + 4 <= data.size();
        ++i)
    {
        if (IsDDS(data, i))
        {
            next = i;
            break;
        }
    }

    size = next - offset;

    return size >= 128;
}

// ============================================================
// ============================================================
// EGS ENCRYPTION
// ============================================================
// Direct port of OpenKH EgsEncryption.cs
// ============================================================

static const uint8_t EGS_MasterKey[0x100] =
{
    0x7E, 0x88, 0x97, 0x55, 0x0B, 0x06, 0xF1, 0x08,
    0xEB, 0xBB, 0x14, 0x1C, 0xD8, 0x7A, 0xEC, 0x41,
    0x34, 0xB2, 0xA3, 0x46, 0xEF, 0x6B, 0xFE, 0xE1,
    0xCF, 0x53, 0xA5, 0x05, 0x12, 0xD2, 0x8E, 0x52,
    0x4A, 0x80, 0xE9, 0x81, 0xB0, 0xF0, 0xB4, 0x9C,
    0xFF, 0x0F, 0x15, 0x13, 0xDA, 0x73, 0x4E, 0x77,
    0xBE, 0xD7, 0x30, 0xE5, 0xF6, 0x5A, 0x11, 0x37,
    0x67, 0xBC, 0x83, 0x6F, 0x27, 0x76, 0xD0, 0xCD,
    0x69, 0x0D, 0x2E, 0x51, 0x42, 0x90, 0xB8, 0xB6,
    0x4C, 0xAD, 0xCE, 0x5B, 0x1A, 0x1F, 0xF5, 0xAF,
    0x01, 0xF8, 0x5E, 0x3A, 0x6E, 0x68, 0x8B, 0xE8,
    0x9F, 0xC9, 0xD9, 0x26, 0x92, 0x29, 0xC8, 0x33,
    0x98, 0x32, 0x54, 0xD4, 0x44, 0x25, 0x66, 0xAC,
    0x5F, 0x99, 0x21, 0xE4, 0x8F, 0x1D, 0xC2, 0xD5,
    0xA4, 0x62, 0xF9, 0x02, 0x61, 0xDE, 0x59, 0xE7,
    0x07, 0x9A, 0xFA, 0x2F, 0x95, 0x3F, 0x86, 0xD3,
    0x78, 0xA7, 0x75, 0xED, 0xD6, 0x2D, 0x64, 0x87,
    0xBD, 0xC7, 0xC1, 0xAA, 0xF2, 0x8C, 0x17, 0xCB,
    0x31, 0x8A, 0xC3, 0xCC, 0x04, 0xEE, 0x6A, 0xAB,
    0x5C, 0x22, 0x70, 0xCA, 0x9E, 0x71, 0x6D, 0x85,
    0x45, 0x5D, 0xB9, 0xA9, 0xA6, 0x10, 0x47, 0xFB,
    0x82, 0x7D, 0x84, 0x7B, 0xC6, 0xE2, 0x38, 0xFC,
    0x2B, 0x0E, 0x20, 0x9D, 0xC5, 0xF3, 0x39, 0xA8,
    0xA0, 0x65, 0x58, 0x43, 0x7C, 0xE3, 0x36, 0x18,
    0x72, 0x49, 0x79, 0xAE, 0xD1, 0x74, 0x40, 0xC4,
    0x91, 0x4F, 0x24, 0x63, 0xBF, 0xBA, 0x23, 0x96,
    0x50, 0xB3, 0x57, 0xDF, 0x1E, 0x03, 0x48, 0x7F,
    0x35, 0x4D, 0x3E, 0xE6, 0xA1, 0xDD, 0x09, 0x3C,
    0x3D, 0x3B, 0x56, 0x8D, 0x93, 0x2A, 0x9B, 0x4B,
    0x0C, 0x28, 0xB1, 0xE0, 0x60, 0x89, 0x19, 0xDB,
    0x2C, 0xF7, 0x6C, 0xB5, 0x1B, 0x94, 0xC0, 0xDC,
    0xEA, 0xB7, 0x0A, 0xF4, 0x16, 0xFD, 0xA2, 0x00
};

static const uint8_t EGS_ScrambleKey[40] =
{
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80,
    0x1B, 0x36
};

// ============================================================
// Generate EGS key
// ============================================================

static std::vector<uint8_t> GenerateEgsKey(
    const uint8_t* seed,
    size_t seedSize,
    int passCount)
{
    std::vector<uint8_t> finalKey(
        0xB0,
        0);

    static const uint8_t ScrambleKey[40] =
    {
        0x01, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00,
        0x1B, 0x00, 0x00, 0x00,
        0x36, 0x00, 0x00, 0x00
    };

    if (seed != nullptr)
    {
        const size_t copySize =
            std::min(seedSize, finalKey.size());

        for (size_t i = 0;
            i < copySize;
            ++i)
        {
            finalKey[i] =
                seed[i] == 0
                ? static_cast<uint8_t>(i)
                : seed[i];
        }
    }

    for (int i = 0;
        i < passCount * 4;
        ++i)
    {
        uint8_t frame[4];

        const size_t frameOffset =
            0x0C +
            static_cast<size_t>(i) * 4;

        frame[0] = finalKey[frameOffset + 0];
        frame[1] = finalKey[frameOffset + 1];
        frame[2] = finalKey[frameOffset + 2];
        frame[3] = finalKey[frameOffset + 3];

        if ((i % 4) == 0)
        {
            const uint8_t old0 = frame[0];
            const uint8_t old1 = frame[1];
            const uint8_t old2 = frame[2];
            const uint8_t old3 = frame[3];

            frame[0] =
                static_cast<uint8_t>(
                    EGS_MasterKey[old1] ^
                    ScrambleKey[i + 0]);

            frame[1] =
                static_cast<uint8_t>(
                    EGS_MasterKey[old2] ^
                    ScrambleKey[i + 1]);

            frame[2] =
                static_cast<uint8_t>(
                    EGS_MasterKey[old3] ^
                    ScrambleKey[i + 2]);

            frame[3] =
                static_cast<uint8_t>(
                    EGS_MasterKey[old0] ^
                    ScrambleKey[i + 3]);
        }

        const size_t outputOffset =
            0x10 +
            static_cast<size_t>(i) * 4;

        const size_t baseOffset =
            static_cast<size_t>(i) * 4;

        finalKey[outputOffset + 0] =
            static_cast<uint8_t>(
                finalKey[baseOffset + 0] ^
                frame[0]);

        finalKey[outputOffset + 1] =
            static_cast<uint8_t>(
                finalKey[baseOffset + 1] ^
                frame[1]);

        finalKey[outputOffset + 2] =
            static_cast<uint8_t>(
                finalKey[baseOffset + 2] ^
                frame[2]);

        finalKey[outputOffset + 3] =
            static_cast<uint8_t>(
                finalKey[baseOffset + 3] ^
                frame[3]);
    }

    return finalKey;
}

// ============================================================
// Decrypt one EGS chunk
// ============================================================

static void DecryptEgsChunk(
    const std::vector<uint8_t>& key,
    std::vector<uint8_t>& data,
    size_t index,
    int passCount)
{
    constexpr size_t MaxChunkLength = 0x10;

    if (index >= data.size())
        return;

    const size_t chunkLength =
        std::min(
            data.size() - index,
            MaxChunkLength);

    for (int i = passCount;
        i >= 0;
        --i)
    {
        const size_t keyOffset =
            0x10 *
            static_cast<size_t>(i);

        for (size_t j = 0;
            j < chunkLength;
            j += 4)
        {
            if (j + 4 > chunkLength)
                break;

            data[index + j + 0] ^=
                key[keyOffset + j + 0];

            data[index + j + 1] ^=
                key[keyOffset + j + 1];

            data[index + j + 2] ^=
                key[keyOffset + j + 2];

            data[index + j + 3] ^=
                key[keyOffset + j + 3];
        }
    }
}

// ============================================================
// Decrypt first 0x100 bytes
// ============================================================

static void DecryptEgsData(
    std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    int passCount)
{
    const size_t decryptLength =
        std::min(
            data.size(),
            static_cast<size_t>(0x100));

    for (size_t i = 0;
        i < decryptLength;
        i += 0x10)
    {
        DecryptEgsChunk(
            key,
            data,
            i,
            passCount);
    }
}

// ============================================================
// Raw DEFLATE decompression
// ============================================================

static bool DecompressEgsDeflate(
    const std::vector<uint8_t>& compressed,
    size_t expectedSize,
    std::vector<uint8_t>& output)
{
    output.clear();

    if (compressed.size() < 2)
        return false;

    if (expectedSize == 0)
        return true;

    if (expectedSize >
        static_cast<size_t>(
            std::numeric_limits<unsigned int>::max()))
    {
        return false;
    }

    const size_t inputSize =
        compressed.size() - 2;

    if (inputSize >
        static_cast<size_t>(
            std::numeric_limits<unsigned int>::max()))
    {
        return false;
    }

    output.resize(expectedSize);

    mz_stream stream{};

    stream.next_in =
        const_cast<unsigned char*>(
            reinterpret_cast<const unsigned char*>(
                compressed.data() + 2));

    stream.avail_in =
        static_cast<unsigned int>(
            inputSize);

    stream.next_out =
        reinterpret_cast<unsigned char*>(
            output.data());

    stream.avail_out =
        static_cast<unsigned int>(
            expectedSize);

    const int initResult =
        mz_inflateInit2(
            &stream,
            -15);

    if (initResult != MZ_OK)
    {
        output.clear();
        return false;
    }

    int result = MZ_OK;

    while (result == MZ_OK)
    {
        result =
            mz_inflate(
                &stream,
                MZ_FINISH);

        if (result == MZ_STREAM_END)
            break;

        if (result != MZ_OK)
            break;

        if (stream.avail_out == 0)
            break;

        if (stream.avail_in == 0)
            break;
    }

    const size_t totalOut =
        static_cast<size_t>(
            stream.total_out);

    const int endResult =
        mz_inflateEnd(&stream);

    if (endResult != MZ_OK)
    {
        output.clear();
        return false;
    }

    if (result != MZ_STREAM_END)
    {
        output.clear();
        return false;
    }

    if (totalOut != expectedSize)
    {
        output.clear();
        return false;
    }

    return true;
}

// ============================================================
// EGS structures
// ============================================================

struct EgsHeaderNative
{
    int32_t decompressedLength = 0;
    int32_t remasteredAssetCount = 0;
    int32_t compressedLength = 0;
    int32_t creationDate = 0;
};

struct EgsRemasteredEntryNative
{
    std::string name;

    int32_t offset = 0;
    int32_t originalAssetOffset = 0;
    int32_t decompressedLength = 0;
    int32_t compressedLength = 0;
};

// ============================================================
// Read EGS header
// ============================================================

static bool ReadEgsHeader(
    const std::vector<uint8_t>& data,
    size_t base,
    EgsHeaderNative& header)
{
    if (!IsValidRange(
        base,
        0x10,
        data.size()))
    {
        return false;
    }

    header.decompressedLength =
        ReadI32LE(
            data.data() +
            base + 0x00);

    header.remasteredAssetCount =
        ReadI32LE(
            data.data() +
            base + 0x04);

    header.compressedLength =
        ReadI32LE(
            data.data() +
            base + 0x08);

    header.creationDate =
        ReadI32LE(
            data.data() +
            base + 0x0C);

    return true;
}

// ============================================================
// Read EGS remastered entry
// ============================================================

static bool ReadEgsRemasteredEntry(
    const std::vector<uint8_t>& data,
    size_t entryOffset,
    EgsRemasteredEntryNative& entry)
{
    if (!IsValidRange(
        entryOffset,
        0x30,
        data.size()))
    {
        return false;
    }

    entry.name.clear();

    bool foundNull = false;

    for (size_t i = 0;
        i < 0x20;
        ++i)
    {
        const uint8_t c =
            data[entryOffset + i];

        if (c == 0)
        {
            foundNull = true;
            break;
        }

        if (c < 32 || c > 126)
            return false;

        entry.name.push_back(
            static_cast<char>(c));
    }

    if (!foundNull)
    {
        // Printable full-length names are still accepted.
    }

    if (entry.name.empty())
        return false;

    entry.offset =
        ReadI32LE(
            data.data() +
            entryOffset + 0x20);

    entry.originalAssetOffset =
        ReadI32LE(
            data.data() +
            entryOffset + 0x24);

    entry.decompressedLength =
        ReadI32LE(
            data.data() +
            entryOffset + 0x28);

    entry.compressedLength =
        ReadI32LE(
            data.data() +
            entryOffset + 0x2C);

    return true;
}

// ============================================================
// Get stored EGS data length
// ============================================================

static bool GetEgsStoredLength(
    int32_t compressedLength,
    int32_t decompressedLength,
    size_t& length)
{
    length = 0;

    if (decompressedLength < 0)
        return false;

    if (compressedLength < -2)
        return false;

    if (compressedLength >= 0)
    {
        length =
            static_cast<size_t>(
                compressedLength);
    }
    else
    {
        length =
            static_cast<size_t>(
                decompressedLength);
    }

    return true;
}

// ============================================================
// Validate EGS HdAsset
// ============================================================

static bool ValidateEgsHdAssetAt(
    const std::vector<uint8_t>& data,
    size_t base,
    EgsHeaderNative& header,
    std::vector<EgsRemasteredEntryNative>& entries,
    size_t& dataOffset)
{
    entries.clear();
    dataOffset = 0;

    if (!ReadEgsHeader(
        data,
        base,
        header))
    {
        return false;
    }

    if (header.decompressedLength < 0)
        return false;

    if (header.compressedLength < -2)
        return false;

    if (header.remasteredAssetCount <= 0 ||
        header.remasteredAssetCount > 1024)
    {
        return false;
    }

    constexpr size_t HeaderSize = 0x10;
    constexpr size_t EntrySize = 0x30;

    const size_t count =
        static_cast<size_t>(
            header.remasteredAssetCount);

    if (count >
        (data.size() - base) / EntrySize)
    {
        return false;
    }

    if (count >
        (std::numeric_limits<size_t>::max() -
            HeaderSize) /
        EntrySize)
    {
        return false;
    }

    const size_t tableSize =
        HeaderSize +
        count * EntrySize;

    if (!IsValidRange(
        base,
        tableSize,
        data.size()))
    {
        return false;
    }

    entries.reserve(count);

    bool hasKnownResource =
        false;

    for (size_t i = 0;
        i < count;
        ++i)
    {
        EgsRemasteredEntryNative entry;

        const size_t entryOffset =
            base +
            HeaderSize +
            i * EntrySize;

        if (!ReadEgsRemasteredEntry(
            data,
            entryOffset,
            entry))
        {
            return false;
        }

        if (entry.decompressedLength < 0 ||
            entry.compressedLength < -2)
        {
            return false;
        }

        constexpr size_t MaxAssetSize =
            static_cast<size_t>(1024) *
            1024 *
            1024;

        if (static_cast<size_t>(
            entry.decompressedLength) >
            MaxAssetSize)
        {
            return false;
        }

        if (entry.compressedLength >= 0 &&
            static_cast<size_t>(
                entry.compressedLength) >
            MaxAssetSize)
        {
            return false;
        }

        if (HasResourceExtension(entry.name))
            hasKnownResource = true;

        entries.push_back(
            std::move(entry));
    }

    if (!hasKnownResource)
        return false;

    dataOffset =
        base +
        tableSize;

    if (dataOffset > data.size())
        return false;

    size_t originalStoredLength = 0;

    if (!GetEgsStoredLength(
        header.compressedLength,
        header.decompressedLength,
        originalStoredLength))
    {
        return false;
    }

    size_t originalAligned = 0;

    if (!Align16Checked(
        originalStoredLength,
        originalAligned))
    {
        return false;
    }

    if (!IsValidRange(
        dataOffset,
        originalAligned,
        data.size()))
    {
        return false;
    }

    size_t cursor =
        dataOffset +
        originalAligned;

    for (const auto& entry : entries)
    {
        size_t storedLength = 0;

        if (!GetEgsStoredLength(
            entry.compressedLength,
            entry.decompressedLength,
            storedLength))
        {
            return false;
        }

        size_t alignedLength = 0;

        if (!Align16Checked(
            storedLength,
            alignedLength))
        {
            return false;
        }

        if (!IsValidRange(
            cursor,
            alignedLength,
            data.size()))
        {
            return false;
        }

        cursor += alignedLength;
    }

    return true;
}

// ============================================================
// Decode one EGS asset
// ============================================================

static bool DecodeEgsAsset(
    const std::vector<uint8_t>& fileData,
    size_t dataOffset,
    int32_t decompressedLength,
    int32_t compressedLength,
    const std::vector<uint8_t>& key,
    std::vector<uint8_t>& decoded,
    size_t& nextOffset)
{
    decoded.clear();
    nextOffset = dataOffset;

    size_t storedLength = 0;

    if (!GetEgsStoredLength(
        compressedLength,
        decompressedLength,
        storedLength))
    {
        return false;
    }

    size_t alignedLength = 0;

    if (!Align16Checked(
        storedLength,
        alignedLength))
    {
        return false;
    }

    if (!IsValidRange(
        dataOffset,
        alignedLength,
        fileData.size()))
    {
        return false;
    }

    std::vector<uint8_t> raw;

    if (storedLength > 0)
    {
        raw.assign(
            fileData.begin() +
            static_cast<std::ptrdiff_t>(
                dataOffset),
            fileData.begin() +
            static_cast<std::ptrdiff_t>(
                dataOffset +
                storedLength));
    }

    if (compressedLength > -2)
    {
        DecryptEgsData(
            raw,
            key,
            10);
    }

    if (compressedLength > -1)
    {
        if (!DecompressEgsDeflate(
            raw,
            static_cast<size_t>(
                decompressedLength),
            decoded))
        {
            return false;
        }
    }
    else
    {
        if (raw.size() !=
            static_cast<size_t>(
                decompressedLength))
        {
            return false;
        }

        decoded =
            std::move(raw);
    }

    nextOffset =
        dataOffset +
        alignedLength;

    return true;
}

// ============================================================
// Validate decoded resource
// ============================================================

static bool ValidateDecodedResource(
    const std::string& name,
    const std::vector<uint8_t>& data)
{
    if (data.empty())
        return false;

    std::string lower =
        name;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c));
        });

    if (lower.size() >= 4 &&
        lower.compare(
            lower.size() - 4,
            4,
            ".dds") == 0)
    {
        if (data.size() < 128)
            return false;

        if (ReadU32LE(data.data()) !=
            MakeFourCC('D', 'D', 'S', ' '))
        {
            return false;
        }

        if (ReadU32LE(
            data.data() + 4) != 124)
        {
            return false;
        }

        const uint32_t height =
            ReadU32LE(
                data.data() + 12);

        const uint32_t width =
            ReadU32LE(
                data.data() + 16);

        if (width == 0 ||
            height == 0 ||
            width > 16384 ||
            height > 16384)
        {
            return false;
        }

        return true;
    }

    return true;
}

// ============================================================
// Extract EGS HdAsset
// ============================================================

static size_t ExtractEgsHdAssetAt(
    const std::vector<uint8_t>& data,
    size_t base,
    const std::filesystem::path& outputDirectory,
    const std::string& baseName)
{
    EgsHeaderNative header;
    std::vector<EgsRemasteredEntryNative> entries;
    size_t dataOffset = 0;

    if (!ValidateEgsHdAssetAt(
        data,
        base,
        header,
        entries,
        dataOffset))
    {
        return 0;
    }

    std::cout
        << "[KH-EGS] ========================================"
        << std::endl;

    std::cout
        << "[KH-EGS] EGS HdAsset detected @ 0x"
        << std::hex
        << base
        << std::dec
        << std::endl;

    std::cout
        << "[KH-EGS] DecompressedLength: "
        << header.decompressedLength
        << std::endl;

    std::cout
        << "[KH-EGS] CompressedLength: "
        << header.compressedLength
        << std::endl;

    std::cout
        << "[KH-EGS] RemasteredAssetCount: "
        << header.remasteredAssetCount
        << std::endl;

    std::cout
        << "[KH-EGS] Data offset: 0x"
        << std::hex
        << dataOffset
        << std::dec
        << std::endl;

    const std::vector<uint8_t> key =
        GenerateEgsKey(
            data.data() + base,
            0x10,
            10);

    std::cout
        << "[KH-EGS] EGS key generated: "
        << key.size()
        << " bytes"
        << std::endl;

    size_t originalNextOffset = 0;

    std::vector<uint8_t> originalData;

    const bool originalDecoded =
        DecodeEgsAsset(
            data,
            dataOffset,
            header.decompressedLength,
            header.compressedLength,
            key,
            originalData,
            originalNextOffset);

    if (originalDecoded)
    {
        std::cout
            << "[KH-EGS] Original asset decoded: "
            << originalData.size()
            << " bytes"
            << std::endl;
    }
    else
    {
        std::cout
            << "[KH-EGS] Original asset decode FAILED."
            << std::endl;

        std::cout
            << "[KH-EGS] Continuing with remastered assets."
            << std::endl;

        size_t originalStoredLength = 0;
        size_t originalAlignedLength = 0;

        if (!GetEgsStoredLength(
            header.compressedLength,
            header.decompressedLength,
            originalStoredLength) ||
            !Align16Checked(
                originalStoredLength,
                originalAlignedLength))
        {
            std::cout
                << "[KH-EGS] Could not calculate original "
                "physical size."
                << std::endl;

            std::cout
                << "[KH-EGS] ========================================"
                << std::endl;

            return 0;
        }

        originalNextOffset =
            dataOffset +
            originalAlignedLength;
    }

    size_t cursor =
        originalNextOffset;

    size_t extracted = 0;

    for (size_t i = 0;
        i < entries.size();
        ++i)
    {
        const auto& entry =
            entries[i];

        std::cout
            << "[KH-EGS] ----------------------------------------"
            << std::endl;

        std::cout
            << "[KH-EGS] Resource #"
            << (i + 1)
            << ": "
            << entry.name
            << std::endl;

        std::cout
            << "[KH-EGS]   Metadata Offset: 0x"
            << std::hex
            << entry.offset
            << std::dec
            << std::endl;

        std::cout
            << "[KH-EGS]   OriginalAssetOffset: 0x"
            << std::hex
            << entry.originalAssetOffset
            << std::dec
            << std::endl;

        std::cout
            << "[KH-EGS]   DecompressedLength: "
            << entry.decompressedLength
            << std::endl;

        std::cout
            << "[KH-EGS]   CompressedLength: "
            << entry.compressedLength
            << std::endl;

        std::cout
            << "[KH-EGS]   Physical Data @ 0x"
            << std::hex
            << cursor
            << std::dec
            << std::endl;

        size_t nextOffset = 0;

        std::vector<uint8_t> decoded;

        if (!DecodeEgsAsset(
            data,
            cursor,
            entry.decompressedLength,
            entry.compressedLength,
            key,
            decoded,
            nextOffset))
        {
            std::cout
                << "[KH-EGS]   DECODE FAILED"
                << std::endl;

            size_t storedLength = 0;
            size_t alignedLength = 0;

            if (GetEgsStoredLength(
                entry.compressedLength,
                entry.decompressedLength,
                storedLength) &&
                Align16Checked(
                    storedLength,
                    alignedLength))
            {
                cursor += alignedLength;

                std::cout
                    << "[KH-EGS]   Continuing at 0x"
                    << std::hex
                    << cursor
                    << std::dec
                    << std::endl;
            }

            continue;
        }

        std::cout
            << "[KH-EGS]   DECODED: "
            << decoded.size()
            << " bytes"
            << std::endl;

        if (!ValidateDecodedResource(
            entry.name,
            decoded))
        {
            std::cout
                << "[KH-EGS]   INVALID RESOURCE DATA"
                << std::endl;

            cursor =
                nextOffset;

            continue;
        }

        std::string filename =
            SanitizeFilename(
                entry.name);

        if (filename.empty() ||
            filename == "resource")
        {
            filename =
                baseName +
                "_egs_" +
                std::to_string(i + 1) +
                ".bin";
        }

        const auto outputPath =
            MakeUniqueOutputPath(
                outputDirectory,
                filename);

        try
        {
            std::ofstream output(
                outputPath,
                std::ios::binary);

            if (!output.is_open())
            {
                std::cout
                    << "[KH-EGS]   FAILED opening output: "
                    << outputPath.string()
                    << std::endl;

                cursor =
                    nextOffset;

                continue;
            }

            output.write(
                reinterpret_cast<const char*>(
                    decoded.data()),
                static_cast<std::streamsize>(
                    decoded.size()));

            if (!output.good())
            {
                std::cout
                    << "[KH-EGS]   FAILED writing output."
                    << std::endl;

                cursor =
                    nextOffset;

                continue;
            }
        }
        catch (const std::exception& e)
        {
            std::cout
                << "[KH-EGS]   EXCEPTION writing output: "
                << e.what()
                << std::endl;

            cursor =
                nextOffset;

            continue;
        }
        catch (...)
        {
            std::cout
                << "[KH-EGS]   UNKNOWN EXCEPTION writing output."
                << std::endl;

            cursor =
                nextOffset;

            continue;
        }

        ++extracted;

        std::cout
            << "[KH-EGS]   EXTRACTED: "
            << outputPath.filename().string()
            << " size="
            << decoded.size()
            << std::endl;

        cursor =
            nextOffset;
    }

    std::cout
        << "[KH-EGS] ========================================"
        << std::endl;

    std::cout
        << "[KH-EGS] Extracted remastered assets: "
        << extracted
        << " / "
        << entries.size()
        << std::endl;

    std::cout
        << "[KH-EGS] ========================================"
        << std::endl;

    return extracted;
}

// ============================================================
// Find EGS HdAssets
// ============================================================

static std::vector<size_t> FindEgsHdAssets(
    const std::vector<uint8_t>& data)
{
    std::vector<size_t> result;

    if (data.size() < 0x40)
        return result;

    {
        EgsHeaderNative header;
        std::vector<EgsRemasteredEntryNative> entries;
        size_t dataOffset = 0;

        if (ValidateEgsHdAssetAt(
            data,
            0,
            header,
            entries,
            dataOffset))
        {
            result.push_back(0);
            return result;
        }
    }

    for (size_t i = 1;
        i + 0x40 <= data.size();
        ++i)
    {
        EgsHeaderNative header;
        std::vector<EgsRemasteredEntryNative> entries;
        size_t dataOffset = 0;

        if (!ValidateEgsHdAssetAt(
            data,
            i,
            header,
            entries,
            dataOffset))
        {
            continue;
        }

        result.push_back(i);

        if (dataOffset > i)
        {
            const size_t skip =
                dataOffset - 1;

            i =
                std::min(
                    data.size() - 1,
                    skip);
        }
    }

    return result;
}

// ============================================================
// MDLS
// ============================================================

static constexpr size_t MDLS_BASE = 0x80;

static constexpr uint32_t MDLS_MAGIC =
0x4A424F4D;

struct MdlsHeaderNative
{
    int32_t textureInfoOffset = 0;
    int32_t textureInfoSize = 0;

    int32_t textureDataOffset = 0;
    int32_t textureDataSize = 0;

    int32_t clutOffset = 0;
    int32_t clutSize = 0;

    int32_t modelOffset = 0;
    int32_t modelSize = 0;

    int32_t unkOffset = 0;
    int32_t unkSize = 0;
};

struct MdlsModelHeaderNative
{
    int32_t jointCount = 0;
    int32_t jointInfoOffset = 0;
    int32_t boneDataOffset = 0;
    int32_t meshCount = 0;
};

static bool ReadMdlsHeader(
    const std::vector<uint8_t>& data,
    size_t base,
    MdlsHeaderNative& header)
{
    if (!IsValidRange(
        base,
        0x2C,
        data.size()))
    {
        return false;
    }

    header.textureInfoOffset =
        ReadI32LE(data.data() + base + 0x00);

    header.textureInfoSize =
        ReadI32LE(data.data() + base + 0x04);

    header.textureDataOffset =
        ReadI32LE(data.data() + base + 0x08);

    header.textureDataSize =
        ReadI32LE(data.data() + base + 0x0C);

    header.clutOffset =
        ReadI32LE(data.data() + base + 0x10);

    header.clutSize =
        ReadI32LE(data.data() + base + 0x14);

    header.modelOffset =
        ReadI32LE(data.data() + base + 0x18);

    header.modelSize =
        ReadI32LE(data.data() + base + 0x1C);

    header.unkOffset =
        ReadI32LE(data.data() + base + 0x20);

    header.unkSize =
        ReadI32LE(data.data() + base + 0x24);

    return true;
}

static bool ValidateMdlsAt(
    const std::vector<uint8_t>& data,
    size_t mdlsBase,
    size_t& payloadSize)
{
    payloadSize = 0;

    if (!IsValidRange(
        mdlsBase,
        MDLS_BASE + 0x2C,
        data.size()))
    {
        return false;
    }

    if (!IsFourCC(
        data,
        mdlsBase + MDLS_BASE,
        MDLS_MAGIC))
    {
        return false;
    }

    const uint32_t dataSize =
        ReadU32LE(
            data.data() +
            mdlsBase +
            MDLS_BASE +
            4);

    MdlsHeaderNative header;

    if (!ReadMdlsHeader(
        data,
        mdlsBase + MDLS_BASE + 8,
        header))
    {
        return false;
    }

    if (header.textureInfoOffset < 0 ||
        header.textureDataOffset < 0 ||
        header.clutOffset < 0 ||
        header.modelOffset < 0 ||
        header.unkOffset < 0)
    {
        return false;
    }

    if (header.textureInfoSize < 0 ||
        header.textureDataSize < 0 ||
        header.clutSize < 0 ||
        header.modelSize < 0 ||
        header.unkSize < 0)
    {
        return false;
    }

    const size_t relativeMinimum =
        MDLS_BASE + 8 + 0x2C;

    if (dataSize != 0 &&
        dataSize < 0x2C)
    {
        return false;
    }

    const size_t sections[] =
    {
        static_cast<size_t>(header.textureInfoOffset),
        static_cast<size_t>(header.textureDataOffset),
        static_cast<size_t>(header.clutOffset),
        static_cast<size_t>(header.modelOffset),
        static_cast<size_t>(header.unkOffset)
    };

    const size_t sizes[] =
    {
        static_cast<size_t>(header.textureInfoSize),
        static_cast<size_t>(header.textureDataSize),
        static_cast<size_t>(header.clutSize),
        static_cast<size_t>(header.modelSize),
        static_cast<size_t>(header.unkSize)
    };

    bool hasValidSection = false;

    size_t maximumEnd =
        relativeMinimum;

    for (size_t i = 0; i < 5; ++i)
    {
        if (sizes[i] == 0)
            continue;

        const size_t sectionStart =
            mdlsBase +
            MDLS_BASE +
            sections[i];

        if (!IsValidRange(
            sectionStart,
            sizes[i],
            data.size()))
        {
            return false;
        }

        hasValidSection = true;

        const size_t end =
            MDLS_BASE +
            sections[i] +
            sizes[i];

        if (end > maximumEnd)
            maximumEnd = end;
    }

    if (!hasValidSection)
        return false;

    const size_t modelHeaderOffset =
        mdlsBase +
        MDLS_BASE +
        static_cast<size_t>(
            header.modelOffset);

    if (!IsValidRange(
        modelHeaderOffset,
        0x10,
        data.size()))
    {
        return false;
    }

    MdlsModelHeaderNative modelHeader;

    modelHeader.jointCount =
        ReadI32LE(
            data.data() +
            modelHeaderOffset + 0x00);

    modelHeader.jointInfoOffset =
        ReadI32LE(
            data.data() +
            modelHeaderOffset + 0x04);

    modelHeader.boneDataOffset =
        ReadI32LE(
            data.data() +
            modelHeaderOffset + 0x08);

    modelHeader.meshCount =
        ReadI32LE(
            data.data() +
            modelHeaderOffset + 0x0C);

    if (modelHeader.jointCount < 0 ||
        modelHeader.jointCount > 4096)
    {
        return false;
    }

    if (modelHeader.meshCount < 0 ||
        modelHeader.meshCount > 4096)
    {
        return false;
    }

    const size_t meshHeadersSize =
        static_cast<size_t>(
            modelHeader.meshCount) *
        0x10;

    if (!IsValidRange(
        modelHeaderOffset + 0x10,
        meshHeadersSize,
        data.size()))
    {
        return false;
    }

    const size_t modelEnd =
        modelHeaderOffset +
        0x10 +
        meshHeadersSize;

    if (modelEnd > maximumEnd)
        maximumEnd = modelEnd;

    payloadSize =
        maximumEnd - mdlsBase;

    if (dataSize > 0)
    {
        const size_t declaredEnd =
            MDLS_BASE +
            static_cast<size_t>(
                dataSize);

        if (declaredEnd >= maximumEnd &&
            declaredEnd <=
            data.size() - mdlsBase)
        {
            payloadSize =
                std::max(
                    payloadSize,
                    declaredEnd);
        }
    }

    if (payloadSize < MDLS_BASE + 0x2C)
        return false;

    if (!IsValidRange(
        mdlsBase,
        payloadSize,
        data.size()))
    {
        return false;
    }

    return true;
}

static std::vector<size_t> FindMDLSHeaders(
    const std::vector<uint8_t>& data)
{
    std::vector<size_t> result;

    if (data.size() < MDLS_BASE + 4)
        return result;

    for (size_t i = 0;
        i + MDLS_BASE + 4 <= data.size();
        ++i)
    {
        if (!IsFourCC(
            data,
            i + MDLS_BASE,
            MDLS_MAGIC))
        {
            continue;
        }

        size_t size = 0;

        if (ValidateMdlsAt(
            data,
            i,
            size))
        {
            result.push_back(i);
        }
    }

    return result;
}

static size_t ExtractMDLS(
    const std::vector<uint8_t>& data,
    const std::filesystem::path& outputDirectory,
    const std::string& baseName)
{
    const auto positions =
        FindMDLSHeaders(data);

    std::cout
        << "[KH-MDLS] Valid MDLS headers: "
        << positions.size()
        << std::endl;

    size_t extracted = 0;

    for (size_t i = 0;
        i < positions.size();
        ++i)
    {
        size_t size = 0;

        if (!ValidateMdlsAt(
            data,
            positions[i],
            size))
        {
            continue;
        }

        std::ostringstream filename;

        filename
            << baseName
            << "_mdls_"
            << std::setw(3)
            << std::setfill('0')
            << (i + 1)
            << ".mdls";

        const auto outputPath =
            MakeUniqueOutputPath(
                outputDirectory,
                filename.str());

        if (WriteBytes(
            data,
            positions[i],
            size,
            outputPath))
        {
            ++extracted;

            std::cout
                << "[KH-MDLS] EXTRACTED: "
                << outputPath.filename().string()
                << " @ 0x"
                << std::hex
                << positions[i]
                << std::dec
                << " size="
                << size
                << std::endl;
        }
    }

    return extracted;
}

// ============================================================
// WPN
// ============================================================

static constexpr uint32_t WPN_MENV_MAGIC =
MakeFourCC('M', 'E', 'N', 'V');

struct WpnHeaderNative
{
    int32_t unk = 0;
    int32_t mainOffset = 0;
    int32_t menvOffset = 0;
    int32_t fileSize = 0;
};

struct WpnMenvHeaderNative
{
    int32_t tag = 0;
    int32_t modelSize = 0;

    int32_t textureInfoOffset = 0;
    int32_t textureInfoSize = 0;

    int32_t textureDataOffset = 0;
    int32_t textureDataSize = 0;

    int32_t clutOffset = 0;
    int32_t clutSize = 0;

    int32_t modelOffset = 0;
    int32_t modelSize2 = 0;

    int32_t unkOffset = 0;
    int32_t unkSize = 0;

    int32_t unk2Offset = 0;
    int32_t unk2Size = 0;
};

static bool ValidateWpnAt(
    const std::vector<uint8_t>& data,
    size_t wpnBase,
    size_t& payloadSize)
{
    payloadSize = 0;

    if (!IsValidRange(
        wpnBase,
        0x10,
        data.size()))
    {
        return false;
    }

    WpnHeaderNative header;

    header.unk =
        ReadI32LE(
            data.data() +
            wpnBase + 0x00);

    header.mainOffset =
        ReadI32LE(
            data.data() +
            wpnBase + 0x04);

    header.menvOffset =
        ReadI32LE(
            data.data() +
            wpnBase + 0x08);

    header.fileSize =
        ReadI32LE(
            data.data() +
            wpnBase + 0x0C);

    if (header.menvOffset < 0)
        return false;

    const size_t menvOffset =
        wpnBase +
        static_cast<size_t>(
            header.menvOffset);

    if (!IsValidRange(
        menvOffset,
        0x40,
        data.size()))
    {
        return false;
    }

    if (!IsFourCC(
        data,
        menvOffset,
        WPN_MENV_MAGIC))
    {
        return false;
    }

    WpnMenvHeaderNative menv;

    menv.tag =
        ReadI32LE(data.data() + menvOffset + 0x00);

    menv.modelSize =
        ReadI32LE(data.data() + menvOffset + 0x04);

    menv.textureInfoOffset =
        ReadI32LE(data.data() + menvOffset + 0x08);

    menv.textureInfoSize =
        ReadI32LE(data.data() + menvOffset + 0x0C);

    menv.textureDataOffset =
        ReadI32LE(data.data() + menvOffset + 0x10);

    menv.textureDataSize =
        ReadI32LE(data.data() + menvOffset + 0x14);

    menv.clutOffset =
        ReadI32LE(data.data() + menvOffset + 0x18);

    menv.clutSize =
        ReadI32LE(data.data() + menvOffset + 0x1C);

    menv.modelOffset =
        ReadI32LE(data.data() + menvOffset + 0x20);

    menv.modelSize2 =
        ReadI32LE(data.data() + menvOffset + 0x24);

    menv.unkOffset =
        ReadI32LE(data.data() + menvOffset + 0x28);

    menv.unkSize =
        ReadI32LE(data.data() + menvOffset + 0x2C);

    menv.unk2Offset =
        ReadI32LE(data.data() + menvOffset + 0x30);

    menv.unk2Size =
        ReadI32LE(data.data() + menvOffset + 0x34);

    const int32_t fields[] =
    {
        menv.modelSize,
        menv.textureInfoOffset,
        menv.textureInfoSize,
        menv.textureDataOffset,
        menv.textureDataSize,
        menv.clutOffset,
        menv.clutSize,
        menv.modelOffset,
        menv.modelSize2,
        menv.unkOffset,
        menv.unkSize,
        menv.unk2Offset,
        menv.unk2Size
    };

    for (int32_t value : fields)
    {
        if (value < 0)
            return false;
    }

    size_t maximumEnd =
        static_cast<size_t>(
            header.menvOffset) +
        0x40;

    const int32_t offsets[] =
    {
        menv.textureInfoOffset,
        menv.textureDataOffset,
        menv.clutOffset,
        menv.modelOffset,
        menv.unkOffset,
        menv.unk2Offset
    };

    const int32_t sizes[] =
    {
        menv.textureInfoSize,
        menv.textureDataSize,
        menv.clutSize,
        menv.modelSize2,
        menv.unkSize,
        menv.unk2Size
    };

    bool hasValidSection = false;

    for (size_t i = 0; i < 6; ++i)
    {
        if (sizes[i] == 0)
            continue;

        const size_t section =
            menvOffset +
            static_cast<size_t>(
                offsets[i]);

        const size_t sectionSize =
            static_cast<size_t>(
                sizes[i]);

        if (!IsValidRange(
            section,
            sectionSize,
            data.size()))
        {
            return false;
        }

        hasValidSection = true;

        const size_t relativeEnd =
            static_cast<size_t>(
                header.menvOffset) +
            static_cast<size_t>(
                offsets[i]) +
            sectionSize;

        if (relativeEnd > maximumEnd)
            maximumEnd = relativeEnd;
    }

    if (!hasValidSection)
        return false;

    const size_t modelHeader =
        menvOffset +
        static_cast<size_t>(
            menv.modelOffset);

    if (!IsValidRange(
        modelHeader,
        0x10,
        data.size()))
    {
        return false;
    }

    const int32_t jointCount =
        ReadI32LE(
            data.data() +
            modelHeader + 0x00);

    const int32_t meshCount =
        ReadI32LE(
            data.data() +
            modelHeader + 0x0C);

    if (jointCount < 0 ||
        jointCount > 4096)
    {
        return false;
    }

    if (meshCount < 0 ||
        meshCount > 4096)
    {
        return false;
    }

    const size_t meshHeadersSize =
        static_cast<size_t>(
            meshCount) *
        0x10;

    if (!IsValidRange(
        modelHeader + 0x10,
        meshHeadersSize,
        data.size()))
    {
        return false;
    }

    const size_t modelEnd =
        modelHeader +
        0x10 +
        meshHeadersSize;

    if (modelEnd > data.size())
        return false;

    const size_t relativeModelEnd =
        static_cast<size_t>(
            header.menvOffset) +
        static_cast<size_t>(
            menv.modelOffset) +
        0x10 +
        meshHeadersSize;

    if (relativeModelEnd > maximumEnd)
        maximumEnd = relativeModelEnd;

    payloadSize =
        maximumEnd;

    if (header.fileSize > 0)
    {
        const size_t declared =
            static_cast<size_t>(
                header.fileSize);

        if (declared <=
            data.size() - wpnBase &&
            declared >= payloadSize)
        {
            payloadSize = declared;
        }
    }

    if (payloadSize < 0x10)
        return false;

    if (!IsValidRange(
        wpnBase,
        payloadSize,
        data.size()))
    {
        return false;
    }

    return true;
}

static std::vector<size_t> FindWPNHeaders(
    const std::vector<uint8_t>& data)
{
    std::vector<size_t> result;

    if (data.size() < 0x10)
        return result;

    for (size_t i = 0;
        i + 0x10 <= data.size();
        ++i)
    {
        size_t size = 0;

        if (ValidateWpnAt(
            data,
            i,
            size))
        {
            result.push_back(i);
        }
    }

    return result;
}

static size_t ExtractWPN(
    const std::vector<uint8_t>& data,
    const std::filesystem::path& outputDirectory,
    const std::string& baseName)
{
    const auto positions =
        FindWPNHeaders(data);

    std::cout
        << "[KH-WPN] Valid WPN headers: "
        << positions.size()
        << std::endl;

    size_t extracted = 0;

    for (size_t i = 0;
        i < positions.size();
        ++i)
    {
        size_t size = 0;

        if (!ValidateWpnAt(
            data,
            positions[i],
            size))
        {
            continue;
        }

        std::ostringstream filename;

        filename
            << baseName
            << "_wpn_"
            << std::setw(3)
            << std::setfill('0')
            << (i + 1)
            << ".wpn";

        const auto outputPath =
            MakeUniqueOutputPath(
                outputDirectory,
                filename.str());

        if (WriteBytes(
            data,
            positions[i],
            size,
            outputPath))
        {
            ++extracted;

            std::cout
                << "[KH-WPN] EXTRACTED: "
                << outputPath.filename().string()
                << " @ 0x"
                << std::hex
                << positions[i]
                << std::dec
                << " size="
                << size
                << std::endl;
        }
    }

    return extracted;
}

// ============================================================
// CVBL
// ============================================================

struct CvblHeaderNative
{
    uint32_t unk1 = 0;
    uint32_t numMeshes = 0;
    uint16_t numUnknownEntries = 0;
    uint16_t hasUnknownEntries = 0;
    uint32_t unk2 = 0;
};

struct CvblMeshEntryNative
{
    uint16_t unk1 = 0;
    uint16_t jointStyle = 0;
    int32_t material = 0;
    int32_t unk2 = 0;
    uint32_t meshOffset = 0;
};

static bool ValidateCvblAt(
    const std::vector<uint8_t>& data,
    size_t cvblBase,
    size_t& payloadSize)
{
    payloadSize = 0;

    if (!IsValidRange(
        cvblBase,
        0x10,
        data.size()))
    {
        return false;
    }

    CvblHeaderNative header;

    header.unk1 =
        ReadU32LE(
            data.data() +
            cvblBase + 0x00);

    header.numMeshes =
        ReadU32LE(
            data.data() +
            cvblBase + 0x04);

    header.numUnknownEntries =
        ReadU16LE(
            data.data() +
            cvblBase + 0x08);

    header.hasUnknownEntries =
        ReadU16LE(
            data.data() +
            cvblBase + 0x0A);

    header.unk2 =
        ReadU32LE(
            data.data() +
            cvblBase + 0x0C);

    if (header.hasUnknownEntries != 0 &&
        header.hasUnknownEntries != 1)
    {
        return false;
    }

    if (header.numMeshes == 0 ||
        header.numMeshes > 4096)
    {
        return false;
    }

    if (header.numUnknownEntries > 4096)
        return false;

    const size_t meshEntriesOffset =
        0x10 +
        (
            header.hasUnknownEntries == 1
            ? static_cast<size_t>(
                header.numUnknownEntries) *
            0x20
            : 0
            );

    const size_t meshTableSize =
        static_cast<size_t>(
            header.numMeshes) *
        0x10;

    if (!IsValidRange(
        cvblBase +
        meshEntriesOffset,
        meshTableSize,
        data.size()))
    {
        return false;
    }

    size_t maximumEnd =
        meshEntriesOffset +
        meshTableSize;

    bool foundMeshPayload = false;

    for (uint32_t i = 0;
        i < header.numMeshes;
        ++i)
    {
        const size_t entryOffset =
            cvblBase +
            meshEntriesOffset +
            static_cast<size_t>(i) *
            0x10;

        CvblMeshEntryNative entry;

        entry.unk1 =
            ReadU16LE(
                data.data() +
                entryOffset + 0x00);

        entry.jointStyle =
            ReadU16LE(
                data.data() +
                entryOffset + 0x02);

        entry.material =
            ReadI32LE(
                data.data() +
                entryOffset + 0x04);

        entry.unk2 =
            ReadI32LE(
                data.data() +
                entryOffset + 0x08);

        entry.meshOffset =
            ReadU32LE(
                data.data() +
                entryOffset + 0x0C);

        if (entry.jointStyle != 8 &&
            entry.jointStyle != 9 &&
            entry.jointStyle != 10)
        {
            return false;
        }

        const size_t meshOffset =
            cvblBase +
            static_cast<size_t>(
                entry.meshOffset);

        if (!IsValidRange(
            meshOffset,
            0x18,
            data.size()))
        {
            return false;
        }

        const size_t subsectionStart =
            meshOffset + 0x10;

        const uint32_t subsectionType =
            ReadU32LE(
                data.data() +
                subsectionStart);

        const uint32_t subsectionLength =
            ReadU32LE(
                data.data() +
                subsectionStart + 4);

        if (subsectionLength < 8)
            return false;

        if (!IsValidRange(
            subsectionStart,
            subsectionLength,
            data.size()))
        {
            return false;
        }

        if (subsectionType != 1 &&
            subsectionType != 17 &&
            subsectionType != 32768)
        {
            return false;
        }

        const size_t end =
            static_cast<size_t>(
                entry.meshOffset) +
            0x10 +
            subsectionLength;

        if (end > maximumEnd)
            maximumEnd = end;

        foundMeshPayload = true;
    }

    if (!foundMeshPayload)
        return false;

    payloadSize =
        maximumEnd;

    return IsValidRange(
        cvblBase,
        payloadSize,
        data.size());
}

static std::vector<size_t> FindCVBLHeaders(
    const std::vector<uint8_t>& data)
{
    std::vector<size_t> result;

    if (data.size() < 0x10)
        return result;

    for (size_t i = 0;
        i + 0x10 <= data.size();
        ++i)
    {
        size_t size = 0;

        if (ValidateCvblAt(
            data,
            i,
            size))
        {
            result.push_back(i);
        }
    }

    return result;
}

static size_t ExtractCVBL(
    const std::vector<uint8_t>& data,
    const std::filesystem::path& outputDirectory,
    const std::string& baseName)
{
    const auto positions =
        FindCVBLHeaders(data);

    std::cout
        << "[KH-CVBL] Valid CVBL headers: "
        << positions.size()
        << std::endl;

    size_t extracted = 0;

    for (size_t i = 0;
        i < positions.size();
        ++i)
    {
        size_t size = 0;

        if (!ValidateCvblAt(
            data,
            positions[i],
            size))
        {
            continue;
        }

        std::ostringstream filename;

        filename
            << baseName
            << "_cvbl_"
            << std::setw(3)
            << std::setfill('0')
            << (i + 1)
            << ".cvbl";

        const auto outputPath =
            MakeUniqueOutputPath(
                outputDirectory,
                filename.str());

        if (WriteBytes(
            data,
            positions[i],
            size,
            outputPath))
        {
            ++extracted;

            std::cout
                << "[KH-CVBL] EXTRACTED: "
                << outputPath.filename().string()
                << " @ 0x"
                << std::hex
                << positions[i]
                << std::dec
                << " size="
                << size
                << std::endl;
        }
    }

    return extracted;
}

// ============================================================
// Resource filename discovery
// ============================================================

static std::vector<size_t> FindResourceNames(
    const std::vector<uint8_t>& data)
{
    std::vector<size_t> positions;

    for (size_t i = 0;
        i < data.size();
        ++i)
    {
        if (data[i] < 32 ||
            data[i] > 126)
        {
            continue;
        }

        if (i > 0 &&
            data[i - 1] >= 32 &&
            data[i - 1] <= 126)
        {
            continue;
        }

        size_t end = i;

        while (end < data.size() &&
            data[end] >= 32 &&
            data[end] <= 126)
        {
            ++end;
        }

        if (end - i < 5)
            continue;

        const std::string name =
            ReadAsciiString(
                data,
                i);

        if (!HasResourceExtension(name))
            continue;

        positions.push_back(i);
    }

    return positions;
}

// ============================================================
// Diagnostic resource filename scan
// ============================================================

static size_t ScanResourceNames(
    const std::vector<uint8_t>& data)
{
    std::cout
        << "[KH-RESOURCE] ========================================"
        << std::endl;

    const auto positions =
        FindResourceNames(data);

    std::cout
        << "[KH-RESOURCE] Resource names discovered: "
        << positions.size()
        << std::endl;

    for (size_t i = 0;
        i < positions.size();
        ++i)
    {
        const size_t offset =
            positions[i];

        const std::string name =
            ReadAsciiString(
                data,
                offset);

        std::cout
            << "[KH-RESOURCE] #"
            << (i + 1)
            << " @ 0x"
            << std::hex
            << offset
            << std::dec
            << ": "
            << name
            << std::endl;
    }

    std::cout
        << "[KH-RESOURCE] ========================================"
        << std::endl;

    return positions.size();
}

// ============================================================
// Extract literal DDS resources
// ============================================================

static size_t ExtractLiteralDDS(
    const std::vector<uint8_t>& data,
    const std::filesystem::path& outputDirectory,
    const std::string& baseName)
{
    const auto positions =
        FindDDSHeaders(data);

    std::cout
        << "[KH-DDS] Literal DDS headers: "
        << positions.size()
        << std::endl;

    size_t extracted = 0;

    for (size_t i = 0;
        i < positions.size();
        ++i)
    {
        size_t size = 0;

        if (!CalculateDDSSize(
            data,
            positions[i],
            size))
        {
            continue;
        }

        std::ostringstream filename;

        filename
            << baseName
            << "_literal_"
            << std::setw(3)
            << std::setfill('0')
            << (i + 1)
            << ".dds";

        const auto outputPath =
            MakeUniqueOutputPath(
                outputDirectory,
                filename.str());

        if (WriteBytes(
            data,
            positions[i],
            size,
            outputPath))
        {
            ++extracted;

            std::cout
                << "[KH-DDS] EXTRACTED: "
                << outputPath.filename().string()
                << " @ 0x"
                << std::hex
                << positions[i]
                << std::dec
                << " size="
                << size
                << std::endl;
        }
    }

    return extracted;
}

// ============================================================
// ExtractFromFile
// ============================================================

bool EmbeddedAssetExtractor::ExtractFromFile(
    const std::filesystem::path& inputFile,
    const std::filesystem::path& outputDirectory)
{
    std::cout
        << "[KH-BIN] Processing: "
        << inputFile.string()
        << std::endl;

    std::ifstream file(
        inputFile,
        std::ios::binary);

    if (!file.is_open())
    {
        std::cout
            << "[KH-BIN] FAILED to open file."
            << std::endl;

        return false;
    }

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
            << "[KH-BIN] Empty file."
            << std::endl;

        return false;
    }

    if (static_cast<uint64_t>(fileSize) >
        static_cast<uint64_t>(
            std::numeric_limits<size_t>::max()))
    {
        std::cout
            << "[KH-BIN] File is too large."
            << std::endl;

        return false;
    }

    std::vector<uint8_t> data(
        static_cast<size_t>(
            fileSize));

    file.read(
        reinterpret_cast<char*>(
            data.data()),
        static_cast<std::streamsize>(
            data.size()));

    if (!file)
    {
        std::cout
            << "[KH-BIN] FAILED reading file."
            << std::endl;

        return false;
    }

    std::cout
        << "[KH-BIN] File size: "
        << data.size()
        << " bytes"
        << std::endl;

    // ========================================================
    // IMPORTANT:
    //
    // Every source/container file gets its own directory.
    //
    // Example:
    //
    // ExtractedAssets/
    //     xw_ex_5010/
    //         texture.dds
    //         model.mdls
    //
    // This keeps all assets associated with the original
    // container together and makes future repacking much
    // easier.
    // ========================================================

    const std::string baseName =
        SanitizeFilename(
            inputFile.stem().string());

    const std::filesystem::path containerDirectory =
        outputDirectory;

    try
    {
        std::filesystem::create_directories(
            containerDirectory);
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[KH-BIN] Failed creating container output "
            << "directory: "
            << containerDirectory.string()
            << std::endl;

        std::cout
            << "[KH-BIN] Reason: "
            << e.what()
            << std::endl;

        return false;
    }

    std::cout
        << "[KH-BIN] Container output directory: "
        << containerDirectory.string()
        << std::endl;

    size_t totalExtracted = 0;

    // ========================================================
    // PASS 1
    //
    // EGS HD ASSET EXTRACTION
    // ========================================================

    const auto egsPositions =
        FindEgsHdAssets(data);

    std::cout
        << "[KH-EGS] Valid EGS HdAssets: "
        << egsPositions.size()
        << std::endl;

    size_t egsExtracted = 0;

    for (size_t i = 0;
        i < egsPositions.size();
        ++i)
    {
        egsExtracted +=
            ExtractEgsHdAssetAt(
                data,
                egsPositions[i],
                containerDirectory,
                baseName);
    }

    totalExtracted +=
        egsExtracted;

    // ========================================================
    // PASS 2
    //
    // Literal DDS
    // ========================================================

    const size_t literalExtracted =
        ExtractLiteralDDS(
            data,
            containerDirectory,
            baseName);

    totalExtracted +=
        literalExtracted;

    // ========================================================
    // PASS 3
    //
    // Standalone MDLS/WPN/CVBL
    // ========================================================

    const size_t mdlsExtracted =
        ExtractMDLS(
            data,
            containerDirectory,
            baseName);

    totalExtracted +=
        mdlsExtracted;

    const size_t wpnExtracted =
        ExtractWPN(
            data,
            containerDirectory,
            baseName);

    totalExtracted +=
        wpnExtracted;

    const size_t cvblExtracted =
        ExtractCVBL(
            data,
            containerDirectory,
            baseName);

    totalExtracted +=
        cvblExtracted;

    // ========================================================
    // PASS 4
    //
    // Diagnostic resource names
    // ========================================================

    const size_t diagnosticResourceCount =
        ScanResourceNames(data);

    // ========================================================
    // Summary
    // ========================================================

    std::cout
        << "[KH-BIN] ========================================"
        << std::endl;

    std::cout
        << "[KH-BIN] Extraction summary"
        << std::endl;

    std::cout
        << "[KH-BIN]   File: "
        << inputFile.filename().string()
        << std::endl;

    std::cout
        << "[KH-BIN]   Size: "
        << data.size()
        << " bytes"
        << std::endl;

    std::cout
        << "[KH-BIN]   Container folder: "
        << containerDirectory.string()
        << std::endl;

    std::cout
        << "[KH-BIN]   EGS HdAssets detected: "
        << egsPositions.size()
        << std::endl;

    std::cout
        << "[KH-BIN]   EGS remastered assets extracted: "
        << egsExtracted
        << std::endl;

    std::cout
        << "[KH-BIN]   Resource names discovered: "
        << diagnosticResourceCount
        << std::endl;

    std::cout
        << "[KH-BIN]   Literal DDS extracted: "
        << literalExtracted
        << std::endl;

    std::cout
        << "[KH-BIN]   MDLS extracted: "
        << mdlsExtracted
        << std::endl;

    std::cout
        << "[KH-BIN]   WPN extracted: "
        << wpnExtracted
        << std::endl;

    std::cout
        << "[KH-BIN]   CVBL extracted: "
        << cvblExtracted
        << std::endl;

    std::cout
        << "[KH-BIN]   TOTAL FILES WRITTEN: "
        << totalExtracted
        << std::endl;

    if (diagnosticResourceCount > 0 &&
        totalExtracted == 0)
    {
        std::cout
            << "[KH-BIN]   NOTE:"
            << std::endl;

        std::cout
            << "[KH-BIN]   Resource filenames were discovered,"
            << std::endl;

        std::cout
            << "[KH-BIN]   but no actual payload was extracted."
            << std::endl;
    }

    if (totalExtracted > 0)
    {
        std::cout
            << "[KH-BIN]   RESULT: PAYLOAD EXTRACTION SUCCESS"
            << std::endl;
    }
    else
    {
        std::cout
            << "[KH-BIN]   RESULT: NO PAYLOADS EXTRACTED"
            << std::endl;
    }

    std::cout
        << "[KH-BIN] ========================================"
        << std::endl;

    return totalExtracted > 0;
}
