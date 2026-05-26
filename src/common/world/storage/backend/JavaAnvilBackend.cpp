/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "JavaAnvilBackend.hpp"
#include "common/util/CompressionUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/storage/reader/java/JavaLevelDatReader.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

using namespace reader::java;

JavaAnvilBackend::JavaAnvilBackend()
    : m_blockMapper(std::make_unique<JavaBlockStateMapper>())
    , m_biomeMapper(std::make_unique<JavaBiomeMapper>())
    , m_chunkReader(std::make_unique<JavaChunkReader>(*m_blockMapper, *m_biomeMapper))
{}

JavaAnvilBackend::~JavaAnvilBackend()
{
    close();
}

Result<void> JavaAnvilBackend::open(const std::filesystem::path& worldPath)
{
    m_worldPath = worldPath;

    // 检测格式信息
    auto formatResult = SaveFormatDetector::detect(worldPath);
    if (formatResult.failed()) {
        return formatResult.error();
    }
    m_formatInfo = formatResult.value();

    if (m_formatInfo.format != SaveFormat::JavaAnvil) {
        return Error(ErrorCode::InvalidState, "JavaAnvilBackend can only open Java Anvil format worlds");
    }

    spdlog::info("JavaAnvilBackend: Opened Java world at {} ({})", worldPath.string(), m_formatInfo.formatName);
    m_isOpen = true;
    return {};
}

void JavaAnvilBackend::close()
{
    m_regionCache.clear();
    m_isOpen = false;
    spdlog::info("JavaAnvilBackend: Closed");
}

bool JavaAnvilBackend::isOpen() const
{
    return m_isOpen;
}

Result<std::optional<ChunkData>> JavaAnvilBackend::loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    // 计算区域坐标
    i32 regionX = x >> 5; // x / 32
    i32 regionZ = z >> 5; // z / 32

    // 区域内偏移
    i32 localX = x & 31;
    i32 localZ = z & 31;

    RegionFile* region = getOrOpenRegion(regionX, regionZ, dimension);
    if (!region) {
        return std::optional<ChunkData>{};
    }

    if (!region->hasChunk(localX, localZ)) {
        return std::optional<ChunkData>{};
    }

    auto dataResult = region->readChunkData(localX, localZ);
    if (dataResult.failed()) {
        return dataResult.error();
    }

    auto& nbtData = dataResult.value();
    if (nbtData.empty()) {
        return std::optional<ChunkData>{};
    }

    auto chunkResult = m_chunkReader->readChunk(nbtData, x, z, dimension);
    if (chunkResult.failed()) {
        return chunkResult.error();
    }
    auto chunkPtr = chunkResult.value();
    if (!chunkPtr) {
        return std::optional<ChunkData>{};
    }
    return std::optional<ChunkData>(std::move(*chunkPtr));
}

Result<std::vector<ChunkPos>> JavaAnvilBackend::listChunks(DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    std::vector<ChunkPos> chunks;
    std::filesystem::path regionDir = getRegionDir(dimension);

    std::error_code ec;
    if (!std::filesystem::exists(regionDir, ec)) {
        return chunks;
    }

    for (const auto& entry : std::filesystem::directory_iterator(regionDir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string filename = entry.path().filename().string();
        if (!filename.starts_with("r.") || !filename.ends_with(".mca")) {
            continue;
        }

        // 解析 r.X.Z.mca
        std::string coords = filename.substr(2, filename.size() - 6);
        auto dotPos = coords.find('.');
        if (dotPos == std::string::npos) {
            continue;
        }

        try {
            i32 rX = std::stoi(coords.substr(0, dotPos));
            i32 rZ = std::stoi(coords.substr(dotPos + 1));

            RegionFile region(entry.path());
            auto openResult = region.open();
            if (openResult.failed()) {
                continue;
            }

            auto regionChunks = region.listChunks();
            for (const auto& [lx, lz] : regionChunks) {
                chunks.emplace_back(rX * 32 + lx, rZ * 32 + lz);
            }

            region.close();
        }
        catch (...) {
            continue;
        }
    }

    return chunks;
}

Result<std::optional<PlayerSaveData>> JavaAnvilBackend::loadPlayer(const std::string& uuid)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    const std::filesystem::path playerPath = m_worldPath / "playerdata" / fmt::format("{}.dat", uuid);
    std::error_code ec;
    if (!std::filesystem::exists(playerPath, ec)) {
        return std::optional<PlayerSaveData>{};
    }

    std::ifstream file(playerPath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Cannot open player data file: {}", playerPath.string()));
    }

    std::vector<u8> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (compressed.empty()) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Player data file is empty: {}", playerPath.string()));
    }

    auto decompressed = mc::util::decompressGzip(compressed);
    if (decompressed.empty()) {
        return Error(
            ErrorCode::DecompressionFailed, fmt::format("Failed to decompress player data: {}", playerPath.string()));
    }

    std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
    stream >> mc::nbt::contexts::java;
    auto root = mc::nbt::tags::compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to parse player NBT: {}", playerPath.string()));
    }

    auto playerResult = PlayerSaveData::fromNbt(*root);
    if (playerResult.failed()) {
        return playerResult.error();
    }

    auto playerData = playerResult.value();
    if (playerData.uuid.empty()) {
        playerData.uuid = uuid;
    }
    return std::optional<PlayerSaveData>(std::move(playerData));
}

Result<std::vector<std::string>> JavaAnvilBackend::listPlayerUuids()
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    std::vector<std::string> uuids;
    const std::filesystem::path playerDir = m_worldPath / "playerdata";
    std::error_code ec;
    if (!std::filesystem::exists(playerDir, ec)) {
        return uuids;
    }

    for (const auto& entry : std::filesystem::directory_iterator(playerDir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".dat") {
            continue;
        }
        uuids.push_back(entry.path().stem().string());
    }

    return uuids;
}

Result<LevelRuntimeData> JavaAnvilBackend::loadLevelData()
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }
    return JavaLevelDatReader::readRuntimeData(m_worldPath);
}

std::filesystem::path JavaAnvilBackend::getRegionDir(DimensionId dimension) const
{
    switch (dimension) {
        case 0:
            return m_worldPath / "region";
        case -1:
            return m_worldPath / "DIM-1" / "region";
        case 1:
            return m_worldPath / "DIM1" / "region";
        default:
            return m_worldPath / fmt::format("DIM{}", dimension) / "region";
    }
}

RegionFile* JavaAnvilBackend::getOrOpenRegion(i32 regionX, i32 regionZ, DimensionId dimension)
{
    const RegionPosKey key{dimension, regionX, regionZ};
    auto it = m_regionCache.find(key);
    if (it != m_regionCache.end()) {
        return it->second.get();
    }

    std::filesystem::path regionDir = getRegionDir(dimension);
    std::filesystem::path regionPath = regionDir / fmt::format("r.{}.{}.mca", regionX, regionZ);

    std::error_code ec;
    if (!std::filesystem::exists(regionPath, ec)) {
        return nullptr;
    }

    auto region = std::make_unique<RegionFile>(regionPath);
    auto openResult = region->open();
    if (openResult.failed()) {
        spdlog::warn("JavaAnvilBackend: Failed to open region file: {}", openResult.error().message());
        return nullptr;
    }

    auto* ptr = region.get();
    m_regionCache.emplace(key, std::move(region));
    return ptr;
}

} // namespace mc::world::storage
