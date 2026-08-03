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

#include "JavaWorldReader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/reader/java/JavaColumnReader.hpp"
#include "common/world/storage/reader/java/RegionFile.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage::reader::java {

using namespace mc::nbt;
using namespace mc::nbt::tags;

namespace {
const compound_tag* getCompound(const compound_tag& parent, const std::string& name)
{
    auto it = parent.value.find(name);
    if (it == parent.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const compound_tag*>(it->second.get());
}
} // namespace

JavaWorldReader::JavaWorldReader(JavaColumnReader& columnReader)
    : m_columnReader(columnReader)
{}

Result<void> JavaWorldReader::open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo)
{
    m_worldPath = worldPath;
    m_formatInfo = formatInfo;
    m_isOpen = true;
    return {};
}

void JavaWorldReader::close()
{
    m_regionCache.clear();
    m_isOpen = false;
}

Result<std::optional<ChunkData>> JavaWorldReader::readChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Java world reader not open");
    }

    // 计算区域坐标和区域内本地坐标
    const i32 regionX = x >> REGION_SHIFT;
    const i32 regionZ = z >> REGION_SHIFT;
    const i32 localX = x & REGION_MASK;
    const i32 localZ = z & REGION_MASK;

    RegionFile* mainRegion = _getOrOpenRegion(regionX, regionZ, dimension, RegionKind::Main);
    RegionFile* entitiesRegion = _getOrOpenRegion(regionX, regionZ, dimension, RegionKind::Entities);

    const bool hasMainChunk = mainRegion != nullptr && mainRegion->hasChunk(localX, localZ);
    const bool hasEntityChunk = entitiesRegion != nullptr && entitiesRegion->hasChunk(localX, localZ);
    if (!hasMainChunk && !hasEntityChunk) {
        return std::optional<ChunkData>{};
    }

    std::optional<std::vector<u8>> mainData;
    if (hasMainChunk) {
        auto dataResult = mainRegion->readChunkData(localX, localZ);
        if (dataResult.failed()) {
            return dataResult.error();
        }
        mainData = std::move(dataResult.value());
    }

    std::optional<std::vector<u8>> entityData;
    if (hasEntityChunk) {
        auto dataResult = entitiesRegion->readChunkData(localX, localZ);
        if (dataResult.failed()) {
            return dataResult.error();
        }
        entityData = std::move(dataResult.value());
    }

    auto combinedDataResult = _combineColumnData(mainData, entityData);
    if (combinedDataResult.failed()) {
        return combinedDataResult.error();
    }

    return m_columnReader.readColumn(combinedDataResult.value(), x, z, dimension);
}

Result<std::vector<ChunkPos>> JavaWorldReader::listChunks(DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Java world reader not open");
    }

    std::vector<ChunkPos> chunks;
    const std::filesystem::path regionDir = _getRegionDir(dimension, RegionKind::Main);

    std::error_code ec;
    if (!std::filesystem::exists(regionDir, ec)) {
        return chunks;
    }

    for (const auto& entry : std::filesystem::directory_iterator(regionDir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::string filename = entry.path().filename().string();
        if (!filename.starts_with("r.") || !filename.ends_with(".mca")) {
            continue;
        }

        const std::string coords = filename.substr(2, filename.size() - 6);
        const auto dotPos = coords.find('.');
        if (dotPos == std::string::npos) {
            continue;
        }

        try {
            const i32 rX = std::stoi(coords.substr(0, dotPos));
            const i32 rZ = std::stoi(coords.substr(dotPos + 1));

            RegionFile region(entry.path());
            auto openResult = region.open();
            if (openResult.failed()) {
                continue;
            }

            for (const auto& [lx, lz] : region.listChunks()) {
                // 将区域本地坐标转换为全局区块坐标
                chunks.emplace_back(rX * REGION_WIDTH + lx, rZ * REGION_WIDTH + lz);
            }
        }
        catch (...) {
            continue;
        }
    }

    return chunks;
}

std::filesystem::path JavaWorldReader::_getRegionDir(DimensionId dimension, RegionKind kind) const
{
    switch (dimension) {
        case 0:
            return m_worldPath / (kind == RegionKind::Main ? "region" : "entities");
        case -1:
            return m_worldPath / "DIM-1" / (kind == RegionKind::Main ? "region" : "entities");
        case 1:
            return m_worldPath / "DIM1" / (kind == RegionKind::Main ? "region" : "entities");
        default:
            return m_worldPath / fmt::format("DIM{}", dimension) / (kind == RegionKind::Main ? "region" : "entities");
    }
}

RegionFile* JavaWorldReader::_getOrOpenRegion(i32 regionX, i32 regionZ, DimensionId dimension, RegionKind kind)
{
    const JavaRegionPosKey key{
        kind == RegionKind::Main ? dimension : static_cast<DimensionId>(dimension ^ 0x40000000), regionX, regionZ};
    auto it = m_regionCache.find(key);
    if (it != m_regionCache.end()) {
        return it->second.get();
    }

    const std::filesystem::path regionPath =
        _getRegionDir(dimension, kind) / fmt::format("r.{}.{}.mca", regionX, regionZ);
    std::error_code ec;
    if (!std::filesystem::exists(regionPath, ec)) {
        return nullptr;
    }

    auto region = std::make_unique<RegionFile>(regionPath);
    auto openResult = region->open();
    if (openResult.failed()) {
        spdlog::warn("JavaWorldReader: Failed to open region file: {}", openResult.error().message());
        return nullptr;
    }

    auto* ptr = region.get();
    m_regionCache.emplace(key, std::move(region));
    return ptr;
}

Result<std::vector<u8>> JavaWorldReader::_combineColumnData(
    const std::optional<std::vector<u8>>& mainData, const std::optional<std::vector<u8>>& entityData) const
{
    // Java 21w43a（数据版本 2724）起，实体数据独立存储于 entities/ 目录
    if (mainData.has_value() && entityData.has_value() && m_formatInfo.dataVersion >= DATA_VERSION_ENTITIES_SEPARATED) {
        return _mergeEntitiesIntoMain(*mainData, *entityData);
    }
    if (!mainData.has_value() && entityData.has_value() &&
        m_formatInfo.dataVersion >= DATA_VERSION_ENTITIES_SEPARATED) {
        return _createEntityOnlyColumn(*entityData);
    }
    if (mainData.has_value()) {
        return *mainData;
    }
    return Error(ErrorCode::ChunkNotFound, "No Java chunk data found in region or entities files");
}

Result<std::vector<u8>> JavaWorldReader::_mergeEntitiesIntoMain(
    const std::vector<u8>& mainData, const std::vector<u8>& entityData) const
{
    auto mainRootResult = _parseJavaRoot(mainData);
    if (mainRootResult.failed()) {
        return mainRootResult.error();
    }

    auto entityRootResult = _parseJavaRoot(entityData);
    if (entityRootResult.failed()) {
        return entityRootResult.error();
    }

    auto mainRootPtr = mainRootResult.value();
    auto& mainRoot = *mainRootPtr;
    auto entityRootPtr = entityRootResult.value();
    auto& entityRoot = *entityRootPtr;
    const compound_tag* entitySource = getCompound(entityRoot, "Level");
    if (entitySource == nullptr) {
        entitySource = &entityRoot;
    }

    auto entityIt = entitySource->value.find("Entities");
    if (entityIt != entitySource->value.end()) {
        compound_tag* target = dynamic_cast<compound_tag*>(mainRoot.value["Level"].get());
        if (target == nullptr) {
            target = &mainRoot;
        }
        target->value["Entities"] = entityIt->second->copy();
    }

    return _writeJavaRoot(mainRoot);
}

Result<std::vector<u8>> JavaWorldReader::_createEntityOnlyColumn(const std::vector<u8>& entityData) const
{
    auto entityRootResult = _parseJavaRoot(entityData);
    if (entityRootResult.failed()) {
        return entityRootResult.error();
    }

    auto entityRootPtr = entityRootResult.value();
    auto& entityRoot = *entityRootPtr;
    const compound_tag* entitySource = getCompound(entityRoot, "Level");
    if (entitySource == nullptr) {
        entitySource = &entityRoot;
    }

    auto positionIt = entitySource->value.find("Position");
    if (positionIt == entitySource->value.end() || positionIt->second->id() != TagId::IntArray) {
        return Error(ErrorCode::ChunkCorrupted, "Java entities region entry missing Position int array");
    }

    const auto& position = dynamic_cast<const intarray_tag&>(*positionIt->second).value;
    if (position.size() < 2) {
        return Error(ErrorCode::ChunkCorrupted, "Java entities region Position array too short");
    }

    compound_tag root;
    root.put("xPos", static_cast<i32>(position[0]));
    root.put("zPos", static_cast<i32>(position[1]));

    auto entityIt = entitySource->value.find("Entities");
    if (entityIt != entitySource->value.end()) {
        root.value["Entities"] = entityIt->second->copy();
    }

    return _writeJavaRoot(root);
}

Result<std::unique_ptr<compound_tag>> JavaWorldReader::_parseJavaRoot(const std::vector<u8>& nbtData) const
{
    std::istringstream stream(std::string(nbtData.begin(), nbtData.end()));
    stream >> contexts::java;
    auto root = compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to parse Java chunk NBT");
    }
    return root;
}

Result<std::vector<u8>> JavaWorldReader::_writeJavaRoot(const compound_tag& root) const
{
    std::ostringstream stream;
    stream << contexts::java;
    root.write(stream);
    const std::string data = stream.str();
    return std::vector<u8>(data.begin(), data.end());
}

} // namespace mc::world::storage::reader::java
