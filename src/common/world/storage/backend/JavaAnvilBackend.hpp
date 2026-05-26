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

#include "common/world/storage/backend/IStorageBackend.hpp"
#include "common/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "common/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include "common/world/storage/reader/java/JavaChunkReader.hpp"
#include "common/world/storage/reader/java/RegionFile.hpp"
#include <memory>
#include <unordered_map>

namespace mc::world::storage {

/// 区域坐标哈希（用于 unordered_map 缓存）
struct RegionPosKey {
    DimensionId dimension = 0;
    i32 regionX = 0;
    i32 regionZ = 0;

    [[nodiscard]] bool operator==(const RegionPosKey& other) const noexcept
    {
        return dimension == other.dimension && regionX == other.regionX && regionZ == other.regionZ;
    }
};

struct RegionPosHash {
    size_t operator()(const RegionPosKey& pos) const noexcept
    {
        auto h1 = std::hash<i32>{}(pos.dimension);
        auto h2 = std::hash<i32>{}(pos.regionX);
        auto h3 = std::hash<i32>{}(pos.regionZ);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief Java Anvil 格式只读存储后端
 *
 * 通过 .mca 区域文件读取 Java 版世界存档。
 * 支持主世界、下界（DIM-1）、末地（DIM1）。
 */
class JavaAnvilBackend : public IStorageBackend {
public:
    JavaAnvilBackend();
    ~JavaAnvilBackend() override;

    JavaAnvilBackend(const JavaAnvilBackend&) = delete;
    JavaAnvilBackend& operator=(const JavaAnvilBackend&) = delete;

    Result<void> open(const std::filesystem::path& worldPath) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;

    [[nodiscard]] Result<std::optional<ChunkData>> loadChunk(
        ChunkCoord x, ChunkCoord z, DimensionId dimension) override;

    [[nodiscard]] Result<std::vector<ChunkPos>> listChunks(DimensionId dimension) override;

    [[nodiscard]] Result<std::optional<PlayerSaveData>> loadPlayer(const std::string& uuid) override;
    [[nodiscard]] Result<std::vector<std::string>> listPlayerUuids() override;

    [[nodiscard]] Result<LevelRuntimeData> loadLevelData() override;

    [[nodiscard]] SaveFormat format() const override { return SaveFormat::JavaAnvil; }
    [[nodiscard]] const SaveFormatInfo& formatInfo() const override { return m_formatInfo; }
    [[nodiscard]] bool isReadonly() const override { return true; }
    [[nodiscard]] const std::filesystem::path& worldPath() const override { return m_worldPath; }

private:
    /// 获取指定维度的 region 目录路径
    [[nodiscard]] std::filesystem::path getRegionDir(DimensionId dimension) const;

    /// 获取或打开区域文件（使用缓存）
    [[nodiscard]] reader::java::RegionFile* getOrOpenRegion(i32 regionX, i32 regionZ, DimensionId dimension);

    std::filesystem::path m_worldPath;
    SaveFormatInfo m_formatInfo;
    bool m_isOpen = false;

    /// 区域文件缓存：(dimension, regionX, regionZ) → RegionFile
    std::unordered_map<RegionPosKey, std::unique_ptr<reader::java::RegionFile>, RegionPosHash> m_regionCache;

    std::unique_ptr<reader::java::JavaBlockStateMapper> m_blockMapper;
    std::unique_ptr<reader::java::JavaBiomeMapper> m_biomeMapper;
    std::unique_ptr<reader::java::JavaChunkReader> m_chunkReader;
};

} // namespace mc::world::storage
