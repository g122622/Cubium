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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/backend/IStorageBackend.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include "common/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "common/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include "common/world/storage/reader/java/JavaChunkReader.hpp"
#include "common/world/storage/reader/java/JavaColumnReader.hpp"
#include "common/world/storage/reader/java/JavaWorldReader.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::world::storage {

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

    // 移动语义
    JavaAnvilBackend(JavaAnvilBackend&& other) noexcept;
    JavaAnvilBackend& operator=(JavaAnvilBackend&& other) noexcept;

    Result<void> open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo) override;
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
    std::filesystem::path m_worldPath;
    SaveFormatInfo m_formatInfo;
    bool m_isOpen = false;

    std::unique_ptr<reader::java::JavaBlockStateMapper> m_blockMapper;
    std::unique_ptr<reader::java::JavaBiomeMapper> m_biomeMapper;
    std::unique_ptr<reader::java::JavaChunkReader> m_chunkReader;
    std::unique_ptr<reader::java::JavaColumnReader> m_columnReader;
    std::unique_ptr<reader::java::JavaWorldReader> m_worldReader;
};

} // namespace mc::world::storage
