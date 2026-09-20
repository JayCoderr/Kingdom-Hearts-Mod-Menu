#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "HedReader.h"

class PkgReader
{
public:

    bool Open(
        const std::string& path);

    // Extract the exact bytes stored in the PKG.
    // This includes the EGS header and encoded/encrypted data.
    bool Extract(
        const HedReader::Entry& entry,
        const std::string& outputPath);

    // Read the exact stored PKG bytes.
    bool ReadStored(
        const HedReader::Entry& entry,
        std::vector<uint8_t>& data);

    // Decode the EGS container and return the original asset.
    bool ReadOriginal(
        const HedReader::Entry& entry,
        std::vector<uint8_t>& data);

    // Extract the decoded/original asset.
    bool ExtractOriginal(
        const HedReader::Entry& entry,
        const std::string& outputPath);

private:

    struct EgsHeader
    {
        int32_t decompressedLength = 0;
        int32_t remasteredAssetCount = 0;
        int32_t compressedLength = 0;
        int32_t creationDate = 0;
    };

    static constexpr int PASS_COUNT = 10;

    bool ReadEgsHeader(
        const std::vector<uint8_t>& data,
        EgsHeader& header);

    bool DecodeEgs(
        const std::vector<uint8_t>& stored,
        std::vector<uint8_t>& original);

    static bool DecryptEgsData(
        std::vector<uint8_t>& data,
        const uint8_t* seed);

    static bool DecompressDeflate(
        const std::vector<uint8_t>& compressedData,
        int32_t expectedSize,
        std::vector<uint8_t>& decompressedData);

    static uint32_t ReadU32LE(
        const uint8_t* data);

    static int32_t ReadI32LE(
        const uint8_t* data);

    static void GenerateKey(
        const uint8_t* seed,
        uint8_t* finalKey);

private:

    std::ifstream file;
};