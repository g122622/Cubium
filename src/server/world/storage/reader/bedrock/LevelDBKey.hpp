/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace mc::world::storage::reader::bedrock {

/**
 * @brief LevelDB 键生成工具类
 *
 * 用于生成和解析基岩版世界存档中 LevelDB 数据库的键格式。
 * 基岩版使用特定的键格式来存储区块数据、实体、方块实体等信息。
 *
 * 键格式说明：
 * - 区块键: [x(4字节LE)][z(4字节LE)][dimension?(4字节LE)][type(1字节)][y?(1字节)]
 * - 前缀键: [prefix][suffix]
 */
class LevelDBKey {
public:
    /**
     * @brief 区块数据类型枚举
     *
     * 定义基岩版 LevelDB 中存储的不同类型区块数据。
     */
    enum class ChunkType : u8 {
        Data3D = 43,               ///< 3D 数据（高度图和生物群系）
        Version = 44,              ///< 区块版本号
        Data2D = 45,               ///< 2D 数据（高度图和生物群系，新格式）
        Data2DLegacy = 46,         ///< 2D 数据（旧格式）
        SubChunkPrefix = 47,       ///< 子区块前缀（方块数据）
        LegacyTerrain = 48,        ///< 旧版地形数据
        BlockEntity = 49,          ///< 方块实体数据
        Entity = 50,               ///< 实体数据
        PendingTicks = 51,         ///< 待处理的 tick
        BlockExtraData = 52,       ///< 方块额外数据
        BiomeState = 53,           ///< 生物群系状态
        FinalizedState = 54,       ///< 区块最终化状态
        BorderBlocks = 55,         ///< 边界方块
        HardCodedDecorations = 56, ///< 硬编码装饰
    };

    // 主世界维度 ID
    static constexpr DimensionId OVERWORLD_DIMENSION = 0;

    /**
     * @brief 检查输入是否以指定前缀开头
     * @param input 输入字节数组
     * @param prefix 前缀字节数组
     * @return 如果输入以前缀开头则返回 true
     */
    [[nodiscard]] static bool startsWith(const std::vector<u8>& input, const std::vector<u8>& prefix);

    /**
     * @brief 从输入中提取后缀部分
     * @param input 输入字节数组
     * @param prefix 前缀字节数组
     * @return 前缀之后的字符串部分
     */
    [[nodiscard]] static std::string extractSuffix(const std::vector<u8>& input, const std::vector<u8>& prefix);

    /**
     * @brief 生成区块数据键
     * @param dimension 维度 ID
     * @param pos 区块坐标
     * @param type 数据类型
     * @return LevelDB 键字节数组
     */
    [[nodiscard]] static std::vector<u8> key(DimensionId dimension, const ChunkPos& pos, ChunkType type);

    /**
     * @brief 生成子区块数据键
     * @param dimension 维度 ID
     * @param pos 区块坐标
     * @param y 子区块 Y 索引
     * @param type 数据类型
     * @return LevelDB 键字节数组
     */
    [[nodiscard]] static std::vector<u8> key(DimensionId dimension, const ChunkPos& pos, i8 y, ChunkType type);

    /**
     * @brief 生成带前缀的区块键
     * @param prefix 键前缀
     * @param dimension 维度 ID
     * @param pos 区块坐标
     * @return LevelDB 键字节数组
     */
    [[nodiscard]] static std::vector<u8> key(const std::vector<u8>& prefix, DimensionId dimension, const ChunkPos& pos);

    /**
     * @brief 生成带前缀和后缀的键
     * @param prefix 键前缀
     * @param suffix 键后缀
     * @return LevelDB 键字节数组
     */
    [[nodiscard]] static std::vector<u8> key(const std::vector<u8>& prefix, std::string_view suffix);

    /**
     * @brief 生成区块前缀（不含类型字节）
     * @param dimension 维度 ID
     * @param pos 区块坐标
     * @return 区块前缀字节数组
     */
    [[nodiscard]] static std::vector<u8> chunkPrefix(DimensionId dimension, const ChunkPos& pos);

    // ============================================================================
    // 预定义键前缀
    // ============================================================================

    /** @brief 实体键前缀 */
    [[nodiscard]] static const std::vector<u8>& actorPrefix();
    /** @brief 生物群系 ID 表键 */
    [[nodiscard]] static const std::vector<u8>& biomeIdsTable();
    /** @brief 维度独立实体分组前缀 */
    [[nodiscard]] static const std::vector<u8>& digpPrefix();
    /** @brief 维度名称 ID 表键 */
    [[nodiscard]] static const std::vector<u8>& dimensionNameIdTable();
    /** @brief 本地玩家数据键 */
    [[nodiscard]] static const std::vector<u8>& localPlayer();
    /** @brief 地图数据前缀 */
    [[nodiscard]] static const std::vector<u8>& mapPrefix();
    /** @brief 传送门数据键 */
    [[nodiscard]] static const std::vector<u8>& portals();
    /** @brief 位置追踪数据库前缀 */
    [[nodiscard]] static const std::vector<u8>& posTrackDb();
    /** @brief 位置追踪数据库最后 ID 键 */
    [[nodiscard]] static const std::vector<u8>& posTrackDbLastId();
};

} // namespace mc::world::storage::reader::bedrock
