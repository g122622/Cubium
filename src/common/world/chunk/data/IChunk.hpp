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

#include "common/core/Constants.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include <array>
#include <tuple>
#include <vector>

// BlockState 在 mc 命名空间中定义
namespace mc {
class BlockState;
} // namespace mc

// SWMRNibbleArray 在 mc 命名空间中定义
namespace mc {
class SWMRNibbleArray;
} // namespace mc

namespace mc::world::chunk {

// 前向声明
class ChunkSection;

// ============================================================================
// 区块状态枚举
// ============================================================================

enum class ChunkLoadStatus : u8 {
    Empty,      // 空区块，刚创建
    Generating, // 正在生成
    Generated,  // 已生成，完整
    Loaded,     // 已加载到内存
    Unloaded    // 已卸载
};

// ============================================================================
// 区块接口
// ============================================================================

/**
 * @brief 区块接口
 *
 * 定义区块的基本操作接口。
 * 支持 ChunkPrimer（中间状态）和 ChunkData（最终状态）。
 */
class IChunk {
public:
    virtual ~IChunk() = default;

    // === 位置信息 ===
    [[nodiscard]] virtual ChunkCoord x() const = 0;
    [[nodiscard]] virtual ChunkCoord z() const = 0;
    [[nodiscard]] virtual ChunkPos pos() const = 0;

    // === 方块访问 ===
    [[nodiscard]] virtual const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const = 0;
    virtual void setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) = 0;
    [[nodiscard]] virtual u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const = 0;
    virtual void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) = 0;

    // === 区块段访问 ===
    [[nodiscard]] virtual ChunkSection* getSection(i32 index) = 0;
    [[nodiscard]] virtual const ChunkSection* getSection(i32 index) const = 0;
    [[nodiscard]] virtual bool hasSection(i32 index) const = 0;
    virtual ChunkSection* createSection(i32 index) = 0;

    /**
     * @brief 获取所有区块段数组
     * @return 指向区块段指针数组的指针，数组大小为 CHUNK_SECTIONS
     */
    [[nodiscard]] virtual const ChunkSection* const* getSections() const = 0;

    // === 生物群系 ===
    [[nodiscard]] virtual BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const = 0;

    // === 高度图 ===
    [[nodiscard]] virtual BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const = 0;
    virtual void updateHeightmap(
        HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) = 0;

    /**
     * @brief 获取高度图原始值（getFirstAvailable 语义）
     *
     * 返回 Heightmap 内部存储值（最高方块 Y+1，或 NO_BLOCK_SENTINEL 表示该列无方块），
     * 不做"空列与 MIN_BUILD_HEIGHT 处有方块"的歧义合并。
     *
     * getTopBlockY 为了对外提供"方块本身 Y"语义，把空列（NO_BLOCK_SENTINEL）回退为
     * MIN_BUILD_HEIGHT，这使空列与"最低层 Y=MIN_BUILD_HEIGHT 处有方块"无法区分。
     * 需要精确识别空列的调用方（如 HeightmapPlacement，对齐 MC HeightmapPlacement 中
     * k > ctx.getMinY() 的判据）应使用本方法拿到原始值，而非 getTopBlockY+1。
     *
     * 默认返回 NO_BLOCK_SENTINEL（视为空列），具体子类按高度图数据返回。
     */
    [[nodiscard]] virtual BlockCoord getHeightmapFirstAvailable(HeightmapType type, BlockCoord x, BlockCoord z) const
    {
        (void)type;
        (void)x;
        (void)z;
        return Heightmap::NO_BLOCK_SENTINEL;
    }

    // === 状态 ===
    [[nodiscard]] virtual ChunkLoadStatus getStatus() const = 0;
    virtual void setStatus(ChunkLoadStatus status) = 0;

    // === 标记 ===
    [[nodiscard]] virtual bool isModified() const = 0;
    virtual void setModified(bool modified) = 0;

    // ========================================================================
    // Starlight 光照数据接口
    // ========================================================================

    /**
     * @brief 获取天空光照空映射
     * @return 空映射数组，每个元素对应一个区块段是否为空
     */
    [[nodiscard]] virtual const bool* getSkyEmptinessMap() const { return nullptr; }

    /**
     * @brief 设置天空光照空映射
     */
    virtual void setSkyEmptinessMap(const bool* map) { (void)map; }

    /**
     * @brief 获取方块光照空映射
     */
    [[nodiscard]] virtual const bool* getBlockEmptinessMap() const { return nullptr; }

    /**
     * @brief 设置方块光照空映射
     */
    virtual void setBlockEmptinessMap(const bool* map) { (void)map; }

    /**
     * @brief 获取天空光照 Nibble 数组
     * @return Nibble 数组指针数组，索引为 [y - minLightSection]
     */
    [[nodiscard]] virtual SWMRNibbleArray* const* getSkyNibbles() const { return nullptr; }

    /**
     * @brief 设置天空光照 Nibble 数组
     */
    virtual void setSkyNibbles(SWMRNibbleArray* const* nibbles) { (void)nibbles; }

    /**
     * @brief 获取方块光照 Nibble 数组
     */
    [[nodiscard]] virtual SWMRNibbleArray* const* getBlockNibbles() const { return nullptr; }

    /**
     * @brief 设置方块光照 Nibble 数组
     */
    virtual void setBlockNibbles(SWMRNibbleArray* const* nibbles) { (void)nibbles; }

    /**
     * @brief 检查区块是否光照正确
     * 用于判断区块是否可以用于光照计算
     */
    [[nodiscard]] virtual bool isLightCorrect() const { return true; }

    /**
     * @brief 设置区块光照正确状态
     */
    virtual void setLightCorrect(bool correct) { (void)correct; }

    // === 结构引用 ===

    /**
     * @brief 获取与指定区块相交的结构名称和区块坐标列表
     *
     * MC 1.21: 用于 STRUCTURE_REFERENCES 阶段。
     * 返回一个 (structureName, sourceChunkX, sourceChunkZ) 的列表，
     * 表示哪些区块的结构起点与此区块相交。
     *
     * @param cx 要检查相交性的区块 X 坐标
     * @param cz 要检查相交性的区块 Z 坐标
     * @return 相交的结构引用列表
     */
    [[nodiscard]] virtual std::vector<std::tuple<mc::ResourceLocation, ChunkCoord, ChunkCoord>>
    getIntersectingStructures(ChunkCoord cx, ChunkCoord cz) const
    {
        (void)cx;
        (void)cz;
        return {};
    }
};

} // namespace mc::world::chunk

namespace mc {
using ChunkLoadStatus = mc::world::chunk::ChunkLoadStatus;
using IChunk = mc::world::chunk::IChunk;
} // namespace mc
