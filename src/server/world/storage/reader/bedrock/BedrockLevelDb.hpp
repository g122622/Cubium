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
#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

namespace leveldb {
class DB;
}

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版 LevelDB 只读接口
 *
 * 提供对基岩版世界 db/ 目录下 LevelDB 数据库的只读访问。
 * 封装了基岩版特有的键格式（chunkX, chunkZ, dimensionId, type, subChunkY）。
 */
class BedrockLevelDb {
public:
    BedrockLevelDb() = default;
    ~BedrockLevelDb();

    BedrockLevelDb(const BedrockLevelDb&) = delete;
    BedrockLevelDb& operator=(const BedrockLevelDb&) = delete;
    BedrockLevelDb(BedrockLevelDb&& other) noexcept;
    BedrockLevelDb& operator=(BedrockLevelDb&& other) noexcept;

    /**
     * @brief 打开 LevelDB 数据库（只读）
     * @param dbPath db/ 目录路径
     * @return 成功或错误
     */
    Result<void> open(const std::filesystem::path& dbPath);

    /**
     * @brief 关闭数据库
     */
    void close();

    /**
     * @brief 检查是否已打开
     */
    [[nodiscard]] bool isOpen() const;

    /**
     * @brief 读取单个键
     * @param key 键
     * @return 值，不存在返回空 optional
     */
    Result<std::optional<std::vector<u8>>> get(const std::vector<u8>& key);

    /**
     * @brief 遍历指定前缀的所有键值对
     * @param prefix 键前缀
     * @param callback 回调函数，返回 false 停止遍历
     */
    using KeyCallback = std::function<bool(const std::vector<u8>& key, const std::vector<u8>& value)>;
    Result<void> iteratePrefix(const std::vector<u8>& prefix, KeyCallback callback);

    /**
     * @brief 遍历指定区块的所有键值对
     * @param chunkX 区块 X
     * @param chunkZ 区块 Z
     * @param dimension 维度 ID
     * @param callback 回调函数
     */
    Result<void> iterateChunk(i32 chunkX, i32 chunkZ, DimensionId dimension, KeyCallback callback);

    // ========== 基岩版键构建工具 ==========

    /// 基岩版 LevelDB 键类型
    enum class ChunkType : u8 {
        Version = 44,
        Data2D = 45,
        Data2DLegacy = 46,
        SubChunkPrefix = 47,
        LegacyTerrain = 48,
        BlockEntity = 49,
        Entity = 50,
        PendingTicks = 51,
        BlockExtraData = 52,
        BiomeState = 53,
        FinalizedState = 54,
        BorderBlocks = 55,
        HardCodedDecorations = 56,
    };

    /// 构建主世界区块键
    static std::vector<u8> buildKey(i32 chunkX, i32 chunkZ, ChunkType type);

    /// 构建其他维度区块键
    static std::vector<u8> buildKey(i32 chunkX, i32 chunkZ, DimensionId dimension, ChunkType type);

    /// 构建子区块键（主世界）
    static std::vector<u8> buildSubChunkKey(i32 chunkX, i32 chunkZ, ChunkType type, i8 subChunkY);

    /// 构建子区块键（其他维度）
    static std::vector<u8> buildSubChunkKey(
        i32 chunkX, i32 chunkZ, DimensionId dimension, ChunkType type, i8 subChunkY);

    /// 构建区块前缀（用于遍历）
    static std::vector<u8> buildChunkPrefix(i32 chunkX, i32 chunkZ, DimensionId dimension);

    /// 基岩版本地玩家键
    [[nodiscard]] static std::vector<u8> buildLocalPlayerKey();

    /// 基岩版 actorprefix 键前缀
    [[nodiscard]] static std::vector<u8> buildActorPrefix();

private:
    leveldb::DB* m_db = nullptr;
};

} // namespace mc::world::storage::reader::bedrock
