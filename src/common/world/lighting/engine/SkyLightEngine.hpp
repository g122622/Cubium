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
#include "common/world/block/Block.hpp"
#include "common/world/lighting/engine/BaseLightEngine.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include <vector>

namespace mc {

// 前向声明
class CollisionShape;
namespace world::chunk {
class ChunkSection;
}
using world::chunk::ChunkSection;

/**
 * @brief 天空光照引擎
 *
 * 天空光照特殊处理：
 * - 向下传播不衰减
 * - 向其他方向传播衰减1级
 * - 追踪表面位置
 * - Null 区块段传播检查
 * - 延迟光照设置
 */
class SkyStarLightEngine : public StarLightEngine {
public:
    /**
     * @brief 构造函数
     *
     * 天空光照引擎不需要在构造时持有 provider 引用，所有光照操作
     * 通过方法参数接收 StarLightLightingProvider* lightAccess。
     * 这遵循"调用时传递"的设计模式，避免引擎持有过期的 provider 引用。
     */
    SkyStarLightEngine();

    // ========================================================================
    // 公共接口
    // ========================================================================

    /**
     * @brief 检查指定位置的光照
     */
    void checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ) override;

    /**
     * @brief 计算光照值
     */
    [[nodiscard]] i32 calculateLightValue(
        StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ, i32 expected) override;

    /**
     * @brief 传播方块变化
     */
    void propagateBlockChanges(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, const std::vector<BlockPos>& positions) override;

    /**
     * @brief 照亮区块
     */
    void lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks) override;

    /**
     * @brief 检查区块边缘（重写以处理 null 区块段）
     */
    void checkChunkEdges(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 fromSection, i32 toSection) override;

    /**
     * @brief 设置世界引用
     */
    void setWorld(void* world) override;

    /**
     * @brief 获取区块的空映射
     */
    [[nodiscard]] const bool* getEmptinessMap(const IChunk* chunk) const override;

    /**
     * @brief 设置区块的空映射
     */
    void setEmptinessMap(const IChunk* chunk, const bool* map) override;

    /**
     * @brief 获取区块的光照数组
     */
    [[nodiscard]] SWMRNibbleArray* const* getNibblesOnChunk(const IChunk* chunk) const override;

    /**
     * @brief 设置区块的光照数组
     */
    void setNibbles(const IChunk* chunk, SWMRNibbleArray* const* nibbles) override;

    /**
     * @brief 检查区块是否可用
     */
    [[nodiscard]] bool canUseChunk(const IChunk* chunk) const override;

    /**
     * @brief 初始化 Nibble 数组
     */
    void initNibble(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude, bool initRemovedNibbles) override;

    /**
     * @brief 设置 Nibble 为 null
     */
    void setNibbleNull(i32 chunkX, i32 chunkY, i32 chunkZ) override;

    // ========================================================================
    // WorldLightManager 接口
    // ========================================================================

    /**
     * @brief 执行一个 tick 的光照更新
     */
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) override;

    /**
     * @brief 更新区块段状态
     */
    void updateSectionStatus(const SectionPos& pos, bool isEmpty) override;

    /**
     * @brief 获取指定位置的光照等级
     */
    [[nodiscard]] u8 getLightFor(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 设置光照数据
     */
    void setData(const SectionPos& pos, const NibbleArray& array, bool retain) override;

    /**
     * @brief 获取光照数据
     */
    [[nodiscard]] SWMRNibbleArray* getData(const SectionPos& pos) override;

    /**
     * @brief 获取光照数据（只读）
     */
    [[nodiscard]] const SWMRNibbleArray* getData(const SectionPos& pos) const override;

protected:
    /**
     * @brief 初始化 Nibble 数组（内部方法）
     */
    void initNibble(SWMRNibbleArray* currNibble, i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude);

    /**
     * @brief 重写 Nibble 缓存（天空光照特殊处理）
     */
    void rewriteNibbleCacheForSkylight(const IChunk* chunk);

    /**
     * @brief 检查 null 区块段
     * @return 是否需要初始化邻居
     */
    bool checkNullSection(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrudeInitialised);

    /**
     * @brief 获取挤出光照等级
     */
    [[nodiscard]] i32 getLightLevelExtruded(i32 worldX, i32 worldY, i32 worldZ);

    /**
     * @brief 尝试传播天空光照
     *
     * 从 startY 向下传播天空光，遇到不透明方块停止。
     * 天空光传播所需的所有方块数据通过基类缓存系统获取，
     * 不需要直接访问 IWorld。
     *
     * @return 无法传播的最高 Y 坐标
     */
    i32 tryPropagateSkylight(i32 worldX, i32 startY, i32 worldZ, bool extrudeInitialised, bool delayLightSet);

    /**
     * @brief 处理延迟的增亮设置
     */
    void processDelayedIncreases();

    /**
     * @brief 处理延迟的减亮设置
     */
    void processDelayedDecreases();

private:
    // 空映射缓存（每个区块）
    std::vector<bool> m_emptinessMapCache;

    // Null 区块段传播检查缓存
    std::vector<bool> m_nullPropagationCheckCache;

    // 高度图（用于方块变化，大小为 CHUNK_WIDTH * CHUNK_WIDTH）
    std::array<i32, world::CHUNK_WIDTH * world::CHUNK_WIDTH> m_heightMapBlockChange;

    /**
     * @brief 获取发射光照等级（天空光照始终为 0）
     * @note 天空光照没有自发光源，返回值始终为 0
     */
    [[nodiscard]] i32 _getLightEmission(const BlockState* state, i32 x, i32 y, i32 z) const noexcept
    {
        MC_UNUSED(state);
        MC_UNUSED(x);
        MC_UNUSED(y);
        MC_UNUSED(z);
        return 0;
    }
};

} // namespace mc
