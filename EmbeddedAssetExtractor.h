#pragma once

#include <filesystem>
#include <string>
#include <vector>

class EmbeddedAssetExtractor
{
public:

    static bool ExtractFromFile(
        const std::filesystem::path& inputFile,
        const std::filesystem::path& outputDirectory);
};