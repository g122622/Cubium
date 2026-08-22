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
#include <cmath>
#include <limits>

namespace mc::world {

// ============================================================================
// 高度限制（左闭右开区间）
// ============================================================================

constexpr i32 MIN_BUILD_HEIGHT = -64;
constexpr i32 MAX_BUILD_HEIGHT = 320;

// ============================================================================
// 区块尺寸
// ============================================================================

constexpr i32 CHUNK_WIDTH = 16;
constexpr i32 CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT;
constexpr i32 CHUNK_SECTION_HEIGHT = 16;
constexpr i32 CHUNK_SECTIONS = CHUNK_HEIGHT / CHUNK_SECTION_HEIGHT;
constexpr i32 CHUNK_VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;

// 区块尺寸（区块内的位偏移）
constexpr i32 CHUNK_SHIFT = 4; // log2(16) = 4
constexpr i32 SECTION_SHIFT = 4;
constexpr i32 CHUNK_MASK = CHUNK_WIDTH - 1;

// 段坐标范围（段坐标 = worldY >> SECTION_SHIFT，区别于段索引 0..CHUNK_SECTIONS-1）
constexpr i32 MIN_SECTION_Y = MIN_BUILD_HEIGHT >> SECTION_SHIFT;       // -4
constexpr i32 MAX_SECTION_Y = (MAX_BUILD_HEIGHT - 1) >> SECTION_SHIFT; // 19

// ============================================================================
// 海平面高度
// ============================================================================

constexpr i32 SEA_LEVEL = 63;

// ============================================================================
// 区块加载
// ============================================================================

constexpr i32 CHUNK_LOAD_RADIUS = 10;
constexpr i32 CHUNK_UNLOAD_RADIUS = 12;
constexpr i32 MAX_CHUNKS_LOADED = 1024;

// ============================================================================
// 世界生成
// ============================================================================

constexpr i64 WORLD_SEED_DEFAULT = 0;
constexpr i32 SPAWN_CHUNK_RADIUS = 11;

// ============================================================================
// 方块更新
// ============================================================================

constexpr i32 BLOCK_UPDATE_RADIUS = 16;

// ============================================================================
// 世界边界
// ============================================================================

/// 世界边界半径（方块坐标绝对值上限）
/// MC 1.16.5: 世界边界为 ±30,000,000
constexpr i32 WORLD_BORDER = 30000000;

/// 可生成实体的世界坐标边界（对齐 Java Level.isInSpawnableBounds）。
/// Java 1.21.11 Level.java:170-186：
///   isInSpawnableBounds = !isOutsideSpawnableHeight(y) && isInWorldBoundsHorizontal(x,z)
///   isOutsideSpawnableHeight(y) = y < -20_000_000 || y >= 20_000_000
///   isInWorldBoundsHorizontal = x/z ∈ [-30_000_000, 30_000_000)
///
/// 注意：此处 Y 边界用硬编码 ±20,000,000（远超实际建筑高度 ±64..320），是防止"超远坐标"
/// 的安全校验，与 MIN/MAX_BUILD_HEIGHT（区块可放置方块高度）语义不同——后者由 isValidY 判定。
/// SummonCommand.createEntity（SummonCommand.java:83-85）首行即用此守卫拦截越界坐标，
/// 防止 setPosition/spawnEntity 处理非法坐标时崩溃或产生越界实体。
constexpr i32 SPAWNABLE_HEIGHT_LIMIT = 20000000;

/// 检查方块坐标是否在可生成实体的世界边界内（对齐 Java Level.isInSpawnableBounds）。
inline bool isInSpawnableBounds(i32 x, i32 y, i32 z) noexcept
{
    const bool heightOk = y >= -SPAWNABLE_HEIGHT_LIMIT && y < SPAWNABLE_HEIGHT_LIMIT;
    const bool horizontalOk = x >= -WORLD_BORDER && z >= -WORLD_BORDER && x < WORLD_BORDER && z < WORLD_BORDER;
    return heightOk && horizontalOk;
}

// ============================================================================
// 区块加载优先级
// ============================================================================

enum class ChunkLoadPriority : i32 {
    Critical = 0,  // 玩家所在区块
    High = 1,      // 玩家周围区块
    Normal = 2,    // 正常加载
    Low = 3,       // 远处区块
    Background = 4 // 后台生成
};

// ============================================================================
// 区块卸载延迟 (毫秒)
// ============================================================================

constexpr u32 CHUNK_UNLOAD_DELAY_MS = 30000; // 30秒

// 区块保存间隔 (毫秒)
constexpr u32 CHUNK_SAVE_INTERVAL_MS = 60000; // 1分钟

// ============================================================================
// 地形生成参数
// ============================================================================

constexpr f32 TERRAIN_HEIGHT_VARIATION = 16.0f;
constexpr f32 TERRAIN_BASE_HEIGHT = 64.0f;
constexpr f32 CAVE_FREQUENCY = 0.02f;
constexpr f32 ORE_FREQUENCY = 0.01f;

// ============================================================================
// 光照更新距离
// ============================================================================

constexpr i32 LIGHT_UPDATE_DISTANCE = 15;

// ============================================================================
// 方块更新传播距离
// ============================================================================

constexpr i32 BLOCK_UPDATE_DISTANCE = 64;

// 红石更新延迟 (ticks)
constexpr i32 REDSTONE_DELAY = 2;

// ============================================================================
// 实体激活范围
// ============================================================================

constexpr i32 ENTITY_ACTIVATION_RANGE_PLAYER = 128;
constexpr i32 ENTITY_ACTIVATION_RANGE_MONSTER = 32;
constexpr i32 ENTITY_ACTIVATION_RANGE_ANIMAL = 32;
constexpr i32 ENTITY_ACTIVATION_RANGE_MISC = 16;

// 实体追踪范围
constexpr i32 ENTITY_TRACKING_RANGE_PLAYER = 64;
constexpr i32 ENTITY_TRACKING_RANGE_MONSTER = 64;
constexpr i32 ENTITY_TRACKING_RANGE_ANIMAL = 48;
constexpr i32 ENTITY_TRACKING_RANGE_MISC = 32;

// 实体消失范围
constexpr i32 ENTITY_DESPAWN_RANGE = 128;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查Y坐标是否在有效范围内
 * @param y Y坐标
 * @return 如果y在 [MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT) 范围内返回true
 */
inline bool isValidY(i32 y) noexcept
{
    return y >= MIN_BUILD_HEIGHT && y < MAX_BUILD_HEIGHT;
}

/**
 * @brief 将世界坐标转换为区块坐标
 * @param worldCoord 世界坐标
 * @return 区块坐标
 */
inline i32 toChunkCoord(i32 worldCoord) noexcept
{
    return worldCoord >= 0 ? worldCoord / CHUNK_WIDTH : (worldCoord + 1) / CHUNK_WIDTH - 1;
}

/**
 * @brief 将世界坐标转换为区块内本地坐标
 * @param worldCoord 世界坐标
 * @return 区块内本地坐标 [0, CHUNK_WIDTH)
 */
inline i32 toLocalCoord(i32 worldCoord) noexcept
{
    i32 local = worldCoord % CHUNK_WIDTH;
    return local >= 0 ? local : local + CHUNK_WIDTH;
}

/**
 * @brief 将区块坐标转换为世界坐标（区块左上角）
 * @param chunkCoord 区块坐标
 * @return 世界坐标
 */
inline i32 toWorldCoord(i32 chunkCoord) noexcept
{
    return chunkCoord * CHUNK_WIDTH;
}

/**
 * @brief 将Y坐标转换为区块段索引
 * @param y Y坐标
 * @return 区块段索引
 */
inline i32 toSectionIndex(i32 y) noexcept
{
    return (y - MIN_BUILD_HEIGHT) / CHUNK_SECTION_HEIGHT;
}

/**
 * @brief 将区块段索引转换为Y坐标
 * @param sectionIndex 区块段索引
 * @return 区块段的最低Y坐标
 */
inline i32 sectionToY(i32 sectionIndex) noexcept
{
    return MIN_BUILD_HEIGHT + sectionIndex * CHUNK_SECTION_HEIGHT;
}

/**
 * @brief 将段索引（0..CHUNK_SECTIONS-1）转换为段坐标（MIN_SECTION_Y..MAX_SECTION_Y）
 * @param sectionIndex 段索引
 * @return 段坐标
 */
inline i32 sectionIndexToCoord(i32 sectionIndex) noexcept
{
    return sectionIndex + (MIN_BUILD_HEIGHT >> SECTION_SHIFT);
}

/**
 * @brief 将段坐标（MIN_SECTION_Y..MAX_SECTION_Y）转换为段索引（0..CHUNK_SECTIONS-1）
 * @param sectionCoord 段坐标
 * @return 段索引
 *
 * TODO: 此函数硬编码使用 MIN_BUILD_HEIGHT 计算偏移，对下界（minHeight=0）等非主世界维度
 * 会导致段索引计算错误。例如下界 section Y=0 映射到索引 4 而非 0。当前 ChunkData 的
 * m_sections 数组固定为 CHUNK_SECTIONS=24 大小，架构上假定主世界的段分布。
 * 完整修复需要：(1) ChunkData 支持维度感知的段偏移 (2) ChunkSection 索引计算使用维度特定的 minHeight
 * 参考 DimensionType::fromId(dimension).minHeight() 获取正确的维度最小高度。
 */
inline i32 sectionCoordToIndex(i32 sectionCoord) noexcept
{
    return sectionCoord - (MIN_BUILD_HEIGHT >> SECTION_SHIFT);
}

/**
 * @brief 检查区块坐标是否有效
 * @param chunkX 区块X坐标
 * @param chunkZ 区块Z坐标
 * @return 如果区块坐标在世界边界内返回true
 */
inline bool isValidChunkCoord(i32 chunkX, i32 chunkZ) noexcept
{
    constexpr i32 MIN_CHUNK = -WORLD_BORDER / CHUNK_WIDTH;
    constexpr i32 MAX_CHUNK = WORLD_BORDER / CHUNK_WIDTH;
    return chunkX >= MIN_CHUNK && chunkX <= MAX_CHUNK && chunkZ >= MIN_CHUNK && chunkZ <= MAX_CHUNK;
}

// 检查方块坐标是否在区块范围内
inline bool isValidBlockInChunk(i32 x, i32 y, i32 z) noexcept
{
    return x >= 0 && x < CHUNK_WIDTH && y >= MIN_BUILD_HEIGHT && y < MAX_BUILD_HEIGHT && z >= 0 && z < CHUNK_WIDTH;
}

// 将Y坐标转换为区块段内本地Y坐标
inline i32 toSectionLocalY(i32 y) noexcept
{
    return (y - MIN_BUILD_HEIGHT) % CHUNK_SECTION_HEIGHT;
}

} // namespace mc::world
