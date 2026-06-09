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

#include "common/core/Types.hpp"

namespace mc {
class ChunkStatus;
}

namespace mc::world::chunk {

/**
 * @brief 区块级别与生成状态的双向映射
 *
 * 管理票据级别（ticket level）与区块生成状态（ChunkStatus）之间的转换。
 * 级别越小优先级越高，级别 31-33 对应完全加载的区块，
 * 级别 34-44 对应逐渐减少的生成阶段需求。
 *
 * 级别语义：
 * - 31 (ENTITY_TICKING_LEVEL): 实体可 tick 的完全加载区块
 * - 32 (BLOCK_TICKING_LEVEL): 方块可 tick 的完全加载区块
 * - 33 (FULL_CHUNK_LEVEL): 完全加载区块
 * - 34-44: 生成中间状态，由 generationStatus() 查询
 * - 45+ (MAX_LEVEL + 1): 未加载
 *
 * 关键公式：
 * - RADIUS_AROUND_FULL_CHUNK = FULL 步骤的累积依赖半径（=11）
 * - MAX_LEVEL = 33 + RADIUS_AROUND_FULL_CHUNK (=44)
 * - generationStatus(level) = accumulatedDependencies[level - 33]
 * - byStatus(status) = 33 + accumulatedRadiusOf(status)
 *
 * 注意：多个状态可能映射到同一级别，因为累积依赖列表中
 * 不同半径可能对应相同的状态。例如：
 * - level 33 → FULL/SPAWN/LIGHT（半径 0）
 * - level 34 → INITIALIZE_LIGHT/FEATURES（半径 1）
 * - level 35 → CARVERS/SURFACE/NOISE（半径 2）
 * - level 36 → BIOMES/STRUCTURE_REFERENCES（半径 3）
 * - level 44 → STRUCTURE_STARTS/EMPTY（半径 11）
 */
class ChunkLevel {
public:
    /// 实体 tick 级别
    static constexpr i32 ENTITY_TICKING_LEVEL = 31;
    /// 方块 tick 级别
    static constexpr i32 BLOCK_TICKING_LEVEL = 32;
    /// 完全加载级别
    static constexpr i32 FULL_CHUNK_LEVEL = 33;

    /**
     * @brief FULL 区块周围的最大依赖半径
     *
     * 从 GENERATION_PYRAMID 中 FULL 步骤的 accumulatedDependencies 计算得出。
     * 当前值为 11（STRUCTURE_STARTS 的累积半径，由依赖偏移合并扩展）。
     *
     * @note 首次调用时从 ChunkPyramid 动态计算，后续缓存
     */
    static i32 radiusAroundFullChunk();

    /**
     * @brief 最大区块级别
     *
     * = 33 + radiusAroundFullChunk() = 44
     * 超过此级别的区块被视为未加载。
     */
    static i32 maxLevel();

    /**
     * @brief 从票据级别推导需要的生成状态
     *
     * 级别 33 或更低 → FULL
     * 级别 34 → SPAWN（FULL 前一步）
     * 级别 35 → LIGHT
     * ...依次类推
     * 级别 > maxLevel() → nullptr（不需要生成）
     *
     * @param level 票据级别
     * @return 需要达到的 ChunkStatus，或 nullptr 表示不需要生成
     */
    static const ChunkStatus* generationStatus(i32 level);

    /**
     * @brief 从生成状态推导对应的票据级别
     *
     * FULL → 33
     * SPAWN → 34
     * LIGHT → 35
     * ...依次类推
     * EMPTY → 44
     *
     * @param status 区块生成状态
     * @return 对应的票据级别
     */
    static i32 byStatus(const ChunkStatus& status);

    /**
     * @brief 从距离推导 FULL 区块周围所需的生成状态
     *
     * 距离 0 → FULL
     * 距离 1 → SPAWN
     * 距离 2 → LIGHT
     * ...依次类推
     * 距离 > radiusAroundFullChunk() → nullptr
     *
     * @param distance 到 FULL 区块的棋盘距离
     * @return 距离处需要的 ChunkStatus，或 nullptr
     */
    static const ChunkStatus* getStatusAroundFullChunk(i32 distance);
};

} // namespace mc::world::chunk
