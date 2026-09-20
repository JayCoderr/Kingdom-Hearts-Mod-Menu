#include "PkgReader.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

#include <windows.h>

#include <zlib.h>

#pragma comment(lib, "C:\\Users\\Jaycoder\\Desktop\\TheXFilesBackOnline\\libs\\zlib\\lib\\z.lib")

// ============================================================
// EGS CONSTANTS
// ============================================================

namespace
{
    constexpr size_t EGS_HEADER_SIZE = 0x10;
    constexpr size_t EGS_MAX_DECRYPT_SIZE = 0x100;

    constexpr uint8_t MASTER_KEY[0x100] =
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

    constexpr uint8_t SCRAMBLE_KEY[0x28] =
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

    // --------------------------------------------------------
    // Basic logging
    // --------------------------------------------------------

    void Log(const std::string& message)
    {
        std::cout
            << "[KH][PKG] "
            << message
            << std::endl;
    }

    // --------------------------------------------------------
    // Little-endian helpers
    // --------------------------------------------------------

    uint32_t ReadU32(
        const uint8_t* p)
    {
        return
            static_cast<uint32_t>(p[0]) |
            (static_cast<uint32_t>(p[1]) << 8) |
            (static_cast<uint32_t>(p[2]) << 16) |
            (static_cast<uint32_t>(p[3]) << 24);
    }

    int32_t ReadI32(
        const uint8_t* p)
    {
        return static_cast<int32_t>(
            ReadU32(p));
    }
}


// ============================================================
// OPEN
// ============================================================

bool PkgReader::Open(
    const std::string& path)
{
    file.close();

    file.clear();

    file.open(
        path,
        std::ios::binary);

    if (!file.is_open())
    {
        Log(
            "FAILED to open PKG: " +
            path);

        return false;
    }

    Log(
        "Opened PKG: " +
        path);

    return true;
}


// ============================================================
// READ STORED
//
// This is the exact data stored at the HED offset.
// It includes the EGS header.
// ============================================================

bool PkgReader::ReadStored(
    const HedReader::Entry& entry,
    std::vector<uint8_t>& data)
{
    data.clear();

    if (!file.is_open())
        return false;

    if (entry.offset < 0 ||
        entry.dataLength < 0)
    {
        return false;
    }

    if (entry.dataLength == 0)
    {
        return true;
    }

    file.clear();

    file.seekg(
        static_cast<std::streamoff>(
            entry.offset),
        std::ios::beg);

    if (!file)
        return false;

    data.resize(
        static_cast<size_t>(
            entry.dataLength));

    file.read(
        reinterpret_cast<char*>(
            data.data()),
        static_cast<std::streamsize>(
            data.size()));

    if (!file)
    {
        data.clear();
        return false;
    }

    return true;
}


// ============================================================
// EXTRACT RAW
//
// Preserves your old behavior.
// ============================================================

bool PkgReader::Extract(
    const HedReader::Entry& entry,
    const std::string& outputPath)
{
    std::vector<uint8_t> buffer;

    if (!ReadStored(
        entry,
        buffer))
    {
        return false;
    }

    const std::filesystem::path output(
        outputPath);

    if (!output.parent_path().empty())
    {
        std::filesystem::create_directories(
            output.parent_path());
    }

    std::ofstream out(
        output,
        std::ios::binary);

    if (!out.is_open())
        return false;

    if (!buffer.empty())
    {
        out.write(
            reinterpret_cast<const char*>(
                buffer.data()),
            static_cast<std::streamsize>(
                buffer.size()));
    }

    return out.good();
}


// ============================================================
// READ EGS HEADER
// ============================================================

bool PkgReader::ReadEgsHeader(
    const std::vector<uint8_t>& data,
    EgsHeader& header)
{
    if (data.size() < EGS_HEADER_SIZE)
        return false;

    header.decompressedLength =
        ReadI32(
            data.data() + 0x00);

    header.remasteredAssetCount =
        ReadI32(
            data.data() + 0x04);

    header.compressedLength =
        ReadI32(
            data.data() + 0x08);

    header.creationDate =
        ReadI32(
            data.data() + 0x0C);

    return true;
}


// ============================================================
// GENERATE EGS KEY
//
// Port of OpenKH EgsEncryption.GenerateKey().
// ============================================================

void PkgReader::GenerateKey(
    const uint8_t* seed,
    uint8_t* finalKey)
{
    std::array<uint8_t, 0xB0> key{};

    // Initial seed.
    for (size_t i = 0; i < 0x10; ++i)
    {
        key[i] =
            seed[i] == 0
            ? static_cast<uint8_t>(i)
            : seed[i];
    }

    // 10 passes * 4 frames.
    for (int i = 0;
        i < PASS_COUNT * 4;
        ++i)
    {
        const size_t frameOffset =
            0x0C +
            static_cast<size_t>(i) * 4;

        uint8_t frame[4];

        frame[0] =
            key[frameOffset + 0];

        frame[1] =
            key[frameOffset + 1];

        frame[2] =
            key[frameOffset + 2];

        frame[3] =
            key[frameOffset + 3];

        if ((i % 4) == 0)
        {
            const size_t scrambleOffset =
                static_cast<size_t>(i) * 4;

            const uint8_t f0 =
                frame[0];

            const uint8_t f1 =
                frame[1];

            const uint8_t f2 =
                frame[2];

            const uint8_t f3 =
                frame[3];

            frame[0] =
                static_cast<uint8_t>(
                    MASTER_KEY[f1] ^
                    SCRAMBLE_KEY[scrambleOffset + 0]);

            frame[1] =
                static_cast<uint8_t>(
                    MASTER_KEY[f2] ^
                    SCRAMBLE_KEY[scrambleOffset + 1]);

            frame[2] =
                static_cast<uint8_t>(
                    MASTER_KEY[f3] ^
                    SCRAMBLE_KEY[scrambleOffset + 2]);

            frame[3] =
                static_cast<uint8_t>(
                    MASTER_KEY[f0] ^
                    SCRAMBLE_KEY[scrambleOffset + 3]);
        }

        key[0x10 + i * 4 + 0] =
            static_cast<uint8_t>(
                key[i * 4 + 0] ^
                frame[0]);

        key[0x10 + i * 4 + 1] =
            static_cast<uint8_t>(
                key[i * 4 + 1] ^
                frame[1]);

        key[0x10 + i * 4 + 2] =
            static_cast<uint8_t>(
                key[i * 4 + 2] ^
                frame[2]);

        key[0x10 + i * 4 + 3] =
            static_cast<uint8_t>(
                key[i * 4 + 3] ^
                frame[3]);
    }

    std::copy(
        key.begin(),
        key.end(),
        finalKey);
}


// ============================================================
// DECRYPT EGS DATA
//
// OpenKH decrypts only the first 0x100 bytes.
// It performs 11 XOR passes (10 down to 0).
// ============================================================

bool PkgReader::DecryptEgsData(
    std::vector<uint8_t>& data,
    const uint8_t* seed)
{
    if (data.empty())
        return true;

    uint8_t key[0xB0]{};

    GenerateKey(
        seed,
        key);

    const size_t decryptLength =
        std::min(
            data.size(),
            EGS_MAX_DECRYPT_SIZE);

    for (int pass = PASS_COUNT;
        pass >= 0;
        --pass)
    {
        const size_t keyOffset =
            static_cast<size_t>(pass) * 0x10;

        for (size_t index = 0;
            index < decryptLength;
            index += 0x10)
        {
            const size_t chunkLength =
                std::min(
                    static_cast<size_t>(0x10),
                    decryptLength - index);

            for (size_t j = 0;
                j < chunkLength;
                j += 4)
            {
                if (j + 3 >= chunkLength)
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

    return true;
}


// ============================================================
// DEFLATE
//
// OpenKH starts DeflateStream at offset 2.
// This means the first two bytes are skipped.
// ============================================================

bool PkgReader::DecompressDeflate(
    const std::vector<uint8_t>& compressedData,
    int32_t expectedSize,
    std::vector<uint8_t>& decompressedData)
{
    decompressedData.clear();

    if (compressedData.size() < 2)
        return false;

    const size_t inputOffset = 2;

    const size_t inputSize =
        compressedData.size() -
        inputOffset;

    if (inputSize == 0)
        return false;

    uLongf outputSize;

    if (expectedSize > 0)
    {
        outputSize =
            static_cast<uLongf>(
                expectedSize);
    }
    else
    {
        // Start with a reasonable estimate.
        outputSize =
            static_cast<uLongf>(
                std::max(
                    size_t(1024),
                    inputSize * 4));
    }

    std::vector<uint8_t> output(
        static_cast<size_t>(
            outputSize));

    const int maxAttempts = 16;

    for (int attempt = 0;
        attempt < maxAttempts;
        ++attempt)
    {
        output.resize(
            static_cast<size_t>(
                outputSize));

        uLongf destinationLength =
            outputSize;

        const int result =
            ::uncompress(
                output.data(),
                &destinationLength,
                compressedData.data() + inputOffset,
                static_cast<uLong>(
                    inputSize));

        if (result == Z_OK)
        {
            output.resize(
                static_cast<size_t>(
                    destinationLength));

            decompressedData =
                std::move(output);

            return true;
        }

        if (result != Z_BUF_ERROR)
        {
            Log(
                "Deflate decompression failed. zlib error=" +
                std::to_string(result));

            return false;
        }

        // Output buffer was too small.
        if (outputSize >
            static_cast<uLongf>(
                std::numeric_limits<size_t>::max() / 2))
        {
            return false;
        }

        outputSize *= 2;
    }

    return false;
}


// ============================================================
// DECODE EGS
// ============================================================

bool PkgReader::DecodeEgs(
    const std::vector<uint8_t>& stored,
    std::vector<uint8_t>& original)
{
    original.clear();

    if (stored.size() < EGS_HEADER_SIZE)
    {
        Log(
            "EGS entry is smaller than 16-byte header.");

        return false;
    }

    EgsHeader header;

    if (!ReadEgsHeader(
        stored,
        header))
    {
        Log(
            "Failed reading EGS header.");

        return false;
    }

    Log(
        "EGS header: "
        "decompressed=" +
        std::to_string(
            header.decompressedLength) +
        ", remasteredCount=" +
        std::to_string(
            header.remasteredAssetCount) +
        ", compressed=" +
        std::to_string(
            header.compressedLength) +
        ", creationDate=" +
        std::to_string(
            header.creationDate));

    // --------------------------------------------------------
    // Basic validation.
    // --------------------------------------------------------

    if (header.decompressedLength < 0)
    {
        Log(
            "Invalid EGS decompressed length.");

        return false;
    }

    if (header.remasteredAssetCount < 0)
    {
        Log(
            "Invalid EGS remastered asset count.");

        return false;
    }

    // --------------------------------------------------------
    // The original EGS data begins after:
    //
    //   0x10 byte original header
    //   remasteredCount * 0x30 byte remastered headers
    //
    // OpenKH reads the remastered headers before ReadData().
    // --------------------------------------------------------

    const uint64_t remasteredHeaderBytes =
        static_cast<uint64_t>(
            header.remasteredAssetCount) *
        0x30ull;

    const uint64_t dataOffset64 =
        0x10ull +
        remasteredHeaderBytes;

    if (dataOffset64 >
        static_cast<uint64_t>(
            stored.size()))
    {
        Log(
            "EGS remastered header area exceeds entry size.");

        return false;
    }

    const size_t dataOffset =
        static_cast<size_t>(
            dataOffset64);

    // --------------------------------------------------------
    // Determine stored original-data length.
    //
    // -2 = no compression/encryption
    // -1 = no compression
    // >=0 = compressed
    // --------------------------------------------------------

    size_t dataLength = 0;

    if (header.compressedLength >= 0)
    {
        dataLength =
            static_cast<size_t>(
                header.compressedLength);
    }
    else
    {
        dataLength =
            static_cast<size_t>(
                header.decompressedLength);
    }

    if (dataOffset >
        stored.size())
    {
        return false;
    }

    if (dataLength >
        stored.size() - dataOffset)
    {
        Log(
            "EGS data length exceeds PKG entry.");

        return false;
    }

    // --------------------------------------------------------
    // Copy original stored data.
    // --------------------------------------------------------

    std::vector<uint8_t> packedData(
        stored.begin() +
        static_cast<std::ptrdiff_t>(
            dataOffset),
        stored.begin() +
        static_cast<std::ptrdiff_t>(
            dataOffset + dataLength));

    // --------------------------------------------------------
    // EGS encryption.
    //
    // OpenKH:
    //
    // if CompressedLength > -2
    //     decrypt first 0x100 bytes
    //
    // The 16-byte original header is the seed.
    // --------------------------------------------------------

    if (header.compressedLength > -2)
    {
        Log(
            "EGS data is encrypted. "
            "Decrypting...");

        if (!DecryptEgsData(
            packedData,
            stored.data()))
        {
            Log(
                "EGS decryption failed.");

            return false;
        }
    }

    // --------------------------------------------------------
    // Compression.
    //
    // >= 0 means compressed.
    // -1 and -2 mean no compression.
    // --------------------------------------------------------

    if (header.compressedLength >= 0)
    {
        Log(
            "EGS data is compressed. "
            "Decompressing...");

        if (!DecompressDeflate(
            packedData,
            header.decompressedLength,
            original))
        {
            Log(
                "EGS decompression failed.");

            return false;
        }
    }
    else
    {
        original =
            std::move(packedData);
    }

    // --------------------------------------------------------
    // Verify expected size when possible.
    // --------------------------------------------------------

    if (header.decompressedLength >= 0)
    {
        if (original.size() !=
            static_cast<size_t>(
                header.decompressedLength))
        {
            Log(
                "WARNING: decoded size mismatch. "
                "Expected=" +
                std::to_string(
                    header.decompressedLength) +
                " Actual=" +
                std::to_string(
                    original.size()));
        }
    }

    return true;
}


// ============================================================
// READ ORIGINAL
// ============================================================

bool PkgReader::ReadOriginal(
    const HedReader::Entry& entry,
    std::vector<uint8_t>& data)
{
    data.clear();

    std::vector<uint8_t> stored;

    if (!ReadStored(
        entry,
        stored))
    {
        Log(
            "Failed reading stored PKG data.");

        return false;
    }

    if (stored.empty())
    {
        return true;
    }

    return DecodeEgs(
        stored,
        data);
}


// ============================================================
// EXTRACT ORIGINAL
// ============================================================

bool PkgReader::ExtractOriginal(
    const HedReader::Entry& entry,
    const std::string& outputPath)
{
    std::vector<uint8_t> data;

    if (!ReadOriginal(
        entry,
        data))
    {
        return false;
    }

    const std::filesystem::path output(
        outputPath);

    if (!output.parent_path().empty())
    {
        std::filesystem::create_directories(
            output.parent_path());
    }

    std::ofstream out(
        output,
        std::ios::binary);

    if (!out.is_open())
        return false;

    if (!data.empty())
    {
        out.write(
            reinterpret_cast<const char*>(
                data.data()),
            static_cast<std::streamsize>(
                data.size()));
    }

    return out.good();
}


// ============================================================
// STATIC HELPERS
// ============================================================

uint32_t PkgReader::ReadU32LE(
    const uint8_t* data)
{
    return
        static_cast<uint32_t>(data[0]) |
        (static_cast<uint32_t>(data[1]) << 8) |
        (static_cast<uint32_t>(data[2]) << 16) |
        (static_cast<uint32_t>(data[3]) << 24);
}


int32_t PkgReader::ReadI32LE(
    const uint8_t* data)
{
    return static_cast<int32_t>(
        ReadU32LE(data));
}