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
 * copies of substantial portions of the Software.
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
#include "common/world/storage/reader/bedrock/BedrockBiomeMapper.hpp"
#include "common/world/storage/reader/bedrock/BedrockChunkReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockColumnReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDb.hpp"
#include "common/world/storage/reader/bedrock/BedrockWorldReader.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 基岩版 LevelDB 格式只读存储后端
 *
 * 通过 LevelDB 数据库读取基岩版世界存档。
 */
class BedrockLDBBackend : public IStorageBackend {
public:
    BedrockLDBBackend();
    ~BedrockLDBBackend() override;

    BedrockLDBBackend(const BedrockLDBBackend&) = delete;
    BedrockLDBBackend& operator=(const BedrockLDBBackend&) = delete;

    Result<void> open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;

    [[nodiscard]] Result<std::optional<ChunkData>> loadChunk(
        ChunkCoord x, ChunkCoord z, DimensionId dimension) override;

    [[nodiscard]] Result<std::vector<ChunkPos>> listChunks(DimensionId dimension) override;

    [[nodiscard]] Result<std::optional<PlayerSaveData>> loadPlayer(const std::string& uuid) override;
    [[nodiscard]] Result<std::vector<std::string>> listPlayerUuids() override;

    [[nodiscard]] Result<LevelRuntimeData> loadLevelData() override;

    [[nodiscard]] SaveFormat format() const override { return SaveFormat::BedrockLDB; }
    [[nodiscard]] const SaveFormatInfo& formatInfo() const override { return m_formatInfo; }
    [[nodiscard]] bool isReadonly() const override { return true; }
    [[nodiscard]] const std::filesystem::path& worldPath() const override { return m_worldPath; }

private:
    std::filesystem::path m_worldPath;
    SaveFormatInfo m_formatInfo;
    bool m_isOpen = false;

    std::unique_ptr<reader::bedrock::BedrockLevelDb> m_db;
    std::unique_ptr<reader::bedrock::BedrockBiomeMapper> m_biomeMapper;
    std::unique_ptr<reader::bedrock::BedrockChunkReader> m_chunkReader;
    std::unique_ptr<reader::bedrock::BedrockColumnReader> m_columnReader;
    std::unique_ptr<reader::bedrock::BedrockWorldReader> m_worldReader;
};

} // namespace mc::world::storage
