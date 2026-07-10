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

#include "common/world/block/BlockPos.hpp"
#include "common/world/lighting/engine/BaseLightEngine.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/world/lighting/storage/EmptinessMap.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"

namespace mc {

// 前向声明
class CollisionShape;
namespace world::chunk {
class ChunkSection;
class ChunkData;
} // namespace world::chunk
using world::chunk::ChunkData;
using world::chunk::ChunkSection;

/**
 * @brief 方块光照引擎
 *
 * 实现方块光照的传播算法：
 * - 光源方块发出初始光照等级
 * - 向所有6个方向传播时都衰减1级
 * - 检测方块的透明度来决定传播衰减
 */
class BlockStarLightEngine : public StarLightEngine {
public:
    /**
     * @brief 构造函数
     *
     * 方块光照引擎不需要在构造时持有 provider 引用，所有光照操作
     * 通过方法参数接收 StarLightLightingProvider* lightAccess。
     * 这遵循"调用时传递"的设计模式，避免引擎持有过期的 provider 引用。
     */
    BlockStarLightEngine();

    // ========================================================================
    // 公共接口
    // ========================================================================

    /**
     * @brief 检查指定位置的光照
     *
     * 方块可以改变透明度、发光等级和传播方向。
     *
     * @param lightAccess 光照区块访问器
     * @param worldX 世界X坐标
     * @param worldY 世界Y坐标
     * @param worldZ 世界Z坐标
     */
    void checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ) override;

    /**
     * @brief 计算光照值
     *
     * 如果结果 > expected，则实际值至少为结果。
     * 如果结果 == expected，则 expected 是正确值。
     * 如果结果 < expected，则结果为实际值。
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
     * @brief 方块发光等级增加时调用
     */
    void onBlockEmissionIncrease(StarLightLightingProvider* lightAccess, i32 x, i32 y, i32 z, i32 lightLevel);

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

    /**
     * @brief 更新区块的空映射
     *
     * 根据区块中每个区块段是否为空来更新空映射。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param chunk 区块数据
     */
    void updateEmptinessMap(i32 chunkX, i32 chunkZ, const ChunkData* chunk);

private:
    // ========================================================================
    // 私有方法
    // ========================================================================

    /**
     * @brief 获取区块的光源位置
     */
    std::vector<BlockPos> _getSources(StarLightLightingProvider* lightAccess, const IChunk* chunk);

    /**
     * @brief 获取发射光照等级
     */
    [[nodiscard]] i32 _getLightEmission(
        StarLightLightingProvider* lightAccess, const BlockState* state, i32 x, i32 y, i32 z) const;

    // ========================================================================
    // 私有成员变量
    // ========================================================================

    // 空映射缓存（每个区块）
    std::vector<bool> m_emptinessMapCache;
};

} // namespace mc
