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

namespace mc::world::chunk {

// ============================================================================
// 区块加载级别
// ============================================================================

/**
 * @brief 区块加载级别
 *
 * Level 越小，优先级越高。
 *
 * 级别说明：
 * - EntityTicking (31): 实体可以 tick 的完全加载区块
 * - BlockTicking (32): 方块可 tick 的完全加载区块
 * - Full (33): 完全加载区块（无 tick）
 * - Border (34): 边界区块（加载但无 tick）
 * - 35-44: 生成中间状态，由 ChunkPyramid::generationStatus() 查询
 *   35 → SPAWN, 36 → LIGHT, 37 → INITIALIZE_LIGHT, 38 → FEATURES, ...
 * - Unloaded (46): 区块未加载
 *
 * @note 级别 31-33 对应 MC 的 FullChunkStatus，级别 34 对应 Border，
 *       级别 35-44 对应生成状态梯度。
 *       MAX_LEVEL = 33 + RADIUS_AROUND_FULL_CHUNK = 44，Unloaded = MAX_LEVEL + 2 = 46。
 */
enum class ChunkLoadLevel : i32 {
    EntityTicking = 31, ///< 实体可以 tick 的完全加载区块
    BlockTicking = 32,  ///< 方块可 tick 的完全加载区块
    Full = 33,          ///< 完全加载区块（无 tick）
    Border = 34,        ///< 边界区块（加载但无 tick）
    // 35-44: 生成中间状态，由 ChunkPyramid::generationStatus() 查询
    Unloaded = 46, ///< 未加载 = ChunkPyramid::maxLevel() + 2
    MaxLevel = 46  ///< 最大级别
};

/**
 * @brief 将视距转换为票据级别
 * @param viewDistance 视距（区块数）
 * @return 票据级别
 *
 * 公式: level = FULL_CHUNK_LEVEL - viewDistance
 * 例如: viewDistance = 10 -> level = 23
 *
 * @note 视距越大，票据级别越小，加载范围越大
 */
inline i32 viewDistanceToLevel(i32 viewDistance)
{
    return static_cast<i32>(ChunkLoadLevel::Full) - viewDistance;
}

/**
 * @brief 检查区块是否应该加载
 * @param level 票据级别
 * @return true 表示区块应该加载（级别 <= Border = 34）
 */
inline bool shouldChunkLoad(i32 level)
{
    return level <= static_cast<i32>(ChunkLoadLevel::Border);
}

// ============================================================================
// 区块加载级别常量（从 ChunkLevel 合并）
// ============================================================================

/// 完全加载级别（与 ChunkLoadLevel::Full 相同）
constexpr i32 FULL_CHUNK_LEVEL = 33;

} // namespace mc::world::chunk
