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
#include <filesystem>
#include <memory>
#include <string>

namespace mc::nbt::tags {
struct compound_tag;
}

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版 level.dat 读取器
 *
 * 独立于现有 LevelDatCodec，专门解析基岩版 level.dat 格式：
 * - 8 字节文件头（4 字节版本 + 4 字节 NBT 数据长度）
 * - 小端序 NBT（contexts::bedrock_disk）
 *
 * 字段名和结构与 Java 版完全不同。
 */
class BedrockLevelDatReader {
public:
    static Result<LevelRuntimeData> readRuntimeData(const std::filesystem::path& worldDir);
    static Result<LevelSummaryData> readSummary(const std::filesystem::path& worldDir);

private:
    static Result<std::unique_ptr<mc::nbt::tags::compound_tag>> _readBedrockNbt(const std::filesystem::path& filePath);

    static Result<LevelSummaryData> _parseSummary(const mc::nbt::tags::compound_tag& root);
    static Result<LevelRuntimeData> _parseRuntimeData(const mc::nbt::tags::compound_tag& root);

    static GameMode _parseGameMode(i32 gameType);
    static Difficulty _parseDifficulty(i32 difficulty);
    static WorldType _parseWorldType(const std::string& generatorName);
    static mc::resource::ResourceLocation _parseWorldPresetId(const std::string& generatorName);
};

} // namespace mc::world::storage::reader::bedrock
