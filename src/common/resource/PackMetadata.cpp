#include "PackMetadata.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace mc {

PackMetadata::PackMetadata(i32 packFormat, std::string description)
    : m_packFormat(packFormat)
    , m_description(std::move(description))
{
}

Result<PackMetadata> PackMetadata::parse(std::string_view jsonContent) {
    try {
        auto json = nlohmann::json::parse(jsonContent);

        PackMetadata metadata;

        if (json.contains("pack")) {
            const auto& pack = json["pack"];

            if (pack.contains("pack_format")) {
                metadata.m_packFormat = pack["pack_format"].get<i32>();
            }

            if (pack.contains("description")) {
                metadata.m_description = pack["description"].get<std::string>();
            }
        }

        return metadata;
    } catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError,
                     std::string("Failed to parse pack.mcmeta: ") + e.what());
    }
}

Result<PackMetadata> PackMetadata::parseFile(std::string_view filePath) {
    std::ifstream file(std::string(filePath), std::ios::binary);

    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, std::string("Cannot open: ") + std::string(filePath));
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());

    return parse(content);
}

bool PackMetadata::isCompatible(i32 minFormat, i32 maxFormat) const {
    return m_packFormat >= minFormat && m_packFormat <= maxFormat;
}

} // namespace mc
