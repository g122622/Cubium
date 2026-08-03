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
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include <filesystem>
#include <memory>
#include <optional>

namespace mc::nbt::tags {
struct compound_tag;
}

namespace mc::world::storage::reader::java {

/**
 * @brief Java 版 level.dat 读取器
 *
 * 独立于现有 LevelDatCodec，专门解析 Java 版 level.dat 格式：
 * - gzip 压缩
 * - 大端序 NBT
 * - Data 复合标签
 *
 * 负责将 Java 版 level.dat 字段转换为项目的 LevelRuntimeData/LevelSummaryData。
 */
class JavaLevelDatReader {
public:
    /**
     * @brief 从 Java 版世界目录读取 level.dat 运行时数据
     * @param worldDir 世界目录路径
     * @return 运行时数据
     */
    static Result<LevelRuntimeData> readRuntimeData(const std::filesystem::path& worldDir);

    /**
     * @brief 从 Java 版世界目录读取 level.dat 摘要数据
     * @param worldDir 世界目录路径
     * @return 摘要数据
     */
    static Result<LevelSummaryData> readSummary(const std::filesystem::path& worldDir);

    /**
     * @brief 从 Java 版世界目录读取 level.dat 中的本地玩家
     * @param worldDir 世界目录路径
     * @return 本地玩家数据；不存在时返回空 optional
     */
    static Result<std::optional<PlayerSaveData>> readLocalPlayer(const std::filesystem::path& worldDir);

private:
    static Result<std::unique_ptr<mc::nbt::tags::compound_tag>> _readGzipNbt(const std::filesystem::path& filePath);

    static Result<LevelSummaryData> _parseSummary(const mc::nbt::tags::compound_tag& data);
    static Result<LevelRuntimeData> _parseRuntimeData(const mc::nbt::tags::compound_tag& data);
    static Result<std::optional<PlayerSaveData>> _parseLocalPlayer(const mc::nbt::tags::compound_tag& data);

    static WorldType _parseWorldType(const mc::nbt::tags::compound_tag& data);
    static resource::ResourceLocation _parseWorldPresetId(const mc::nbt::tags::compound_tag& data);
    static GameMode _parseGameMode(const mc::nbt::tags::compound_tag& data);
    static Difficulty _parseDifficulty(const mc::nbt::tags::compound_tag& data);
};

} // namespace mc::world::storage::reader::java
