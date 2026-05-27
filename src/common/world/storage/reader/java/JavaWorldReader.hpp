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

#pragma once

#include "JavaColumnReader.hpp"
#include "RegionFile.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>

namespace mc::world::storage::reader::java {

struct JavaRegionPosKey {
    DimensionId dimension = 0;
    i32 regionX = 0;
    i32 regionZ = 0;

    [[nodiscard]] bool operator==(const JavaRegionPosKey& other) const noexcept
    {
        return dimension == other.dimension && regionX == other.regionX && regionZ == other.regionZ;
    }
};

struct JavaRegionPosHash {
    [[nodiscard]] size_t operator()(const JavaRegionPosKey& pos) const noexcept
    {
        const auto h1 = std::hash<i32>{}(pos.dimension);
        const auto h2 = std::hash<i32>{}(pos.regionX);
        const auto h3 = std::hash<i32>{}(pos.regionZ);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief Java 世界级读取器
 *
 * 负责 world 级别的 region 文件定位、缓存与列数据获取。
 * 对 1.17+ Java 世界，会在这一层合并 `region/` 与 `entities/` 中的列数据。
 */
class JavaWorldReader {
public:
    explicit JavaWorldReader(JavaColumnReader& columnReader);

    Result<void> open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo);
    void close();
    [[nodiscard]] bool isOpen() const { return m_isOpen; }

    [[nodiscard]] Result<std::optional<ChunkData>> readChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension);
    [[nodiscard]] Result<std::vector<ChunkPos>> listChunks(DimensionId dimension);

private:
    enum class RegionKind : u8 {
        Main,
        Entities,
    };

    [[nodiscard]] std::filesystem::path getRegionDir(DimensionId dimension, RegionKind kind) const;
    [[nodiscard]] RegionFile* getOrOpenRegion(i32 regionX, i32 regionZ, DimensionId dimension, RegionKind kind);
    [[nodiscard]] Result<std::vector<u8>> combineColumnData(
        const std::optional<std::vector<u8>>& mainData, const std::optional<std::vector<u8>>& entityData) const;
    [[nodiscard]] Result<std::vector<u8>> mergeEntitiesIntoMain(
        const std::vector<u8>& mainData, const std::vector<u8>& entityData) const;
    [[nodiscard]] Result<std::vector<u8>> createEntityOnlyColumn(const std::vector<u8>& entityData) const;
    [[nodiscard]] Result<std::unique_ptr<mc::nbt::tags::compound_tag>> parseJavaRoot(
        const std::vector<u8>& nbtData) const;
    [[nodiscard]] Result<std::vector<u8>> writeJavaRoot(const mc::nbt::tags::compound_tag& root) const;

    JavaColumnReader& m_columnReader;
    std::filesystem::path m_worldPath;
    SaveFormatInfo m_formatInfo;
    bool m_isOpen = false;
    std::unordered_map<JavaRegionPosKey, std::unique_ptr<RegionFile>, JavaRegionPosHash> m_regionCache;
};

} // namespace mc::world::storage::reader::java
