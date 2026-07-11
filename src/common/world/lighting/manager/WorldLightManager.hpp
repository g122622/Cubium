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

#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include <memory>
#include <mutex>

namespace mc {

// 前向声明
namespace world::chunk {
class IChunk;
}
using world::chunk::IChunk;
namespace world::chunk {
class ChunkData;
}
using world::chunk::ChunkData;

/**
 * @brief 世界光照管理器
 *
 * 协调方块光照和天空光照引擎，提供统一的光照计算接口。
 * 根据维度配置，可能只有方块光照（如下界）或两者都有（如主世界）。
 */
class WorldLightManager {
public:
    /**
     * @brief 构造函数
     *
     * @param provider 区块光照提供者
     * @param hasBlockLight 是否有方块光照
     * @param hasSkyLight 是否有天空光照
     */
    WorldLightManager(StarLightLightingProvider* provider, bool hasBlockLight, bool hasSkyLight);

    // ========================================================================
    // 光照操作
    // ========================================================================

    /**
     * @brief 检查方块的光照
     *
     * 当方块状态改变时调用，重新计算该位置的光照。
     *
     * @param pos 方块位置
     */
    void checkBlock(i32 x, i32 y, i32 z);

    /**
     * @brief 批量检查一个区块内多个方块的光照
     *
     * 运行时方块变更延迟队列在 tick 时调用：把同一区块内累积的全部变更坐标
     * 一次性传入，仅做一次 setupCaches/destroyCaches 即完成全部传播，
     * 避免逐坐标调用 checkBlock 的重复缓存开销。changedSections 留空，
     * 因运行时方块变更不改变区块段空状态（空状态由区块加载流程维护）。
     *
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param positions 待处理的方块坐标列表
     */
    void checkBlocks(i32 chunkX, i32 chunkZ, std::vector<BlockPos> positions);

    /**
     * @brief 方块发光等级增加时调用
     *
     * 当方块被放置且发光等级大于0时调用。
     *
     * @param pos 方块位置
     * @param lightLevel 发光等级
     */
    void onBlockEmissionIncrease(i32 x, i32 y, i32 z, i32 lightLevel);

    /**
     * @brief 检查是否有待处理的光照工作
     */
    [[nodiscard]] bool hasLightWork() const;

    /**
     * @brief 处理光照更新
     *
     * @param maxUpdates 最大更新数量
     * @param updateSkyLight 是否更新天空光照
     * @param updateBlockLight 是否更新方块光照
     * @return 剩余更新配额
     */
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

    // ========================================================================
    // 区块段管理
    // ========================================================================

    /**
     * @brief 更新区块段状态
     *
     * 当区块段加载/卸载或变为空时调用。
     *
     * @param pos 区块段位置
     * @param isEmpty 是否为空（没有方块）
     */
    void updateSectionStatus(const SectionPos& pos, bool isEmpty);

    // ========================================================================
    // 光照访问
    // ========================================================================

    /**
     * @brief 获取光照引擎
     *
     * @param type 光照类型
     * @return 光照引擎指针，如果不存在返回nullptr
     */
    [[nodiscard]] BlockStarLightEngine* getBlockLightEngine();
    [[nodiscard]] const BlockStarLightEngine* getBlockLightEngine() const;
    [[nodiscard]] SkyStarLightEngine* getSkyLightEngine();
    [[nodiscard]] const SkyStarLightEngine* getSkyLightEngine() const;

    /**
     * @brief 获取指定位置的实际亮度
     *
     * 考虑天空减暗因子，计算最终亮度。
     *
     * @param pos 方块位置
     * @param skyDarkening 天空减暗因子（0-15）
     * @return 亮度值 (0-15)
     */
    [[nodiscard]] i32 getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const;

    /**
     * @brief 获取方块光照等级
     */
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取天空光照等级
     */
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;

    // ========================================================================
    // 数据管理
    // ========================================================================

    /**
     * @brief 设置光照数据
     *
     * 用于从存档加载光照数据。
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @param array 光照数组
     * @param retain 是否保留（防止被覆盖）
     */
    void setData(LightType type, const SectionPos& pos, const NibbleArray& array, bool retain);

    /**
     * @brief 获取光照数据
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @return 光照数组指针，如果不存在返回nullptr
     */
    [[nodiscard]] SWMRNibbleArray* getData(LightType type, const SectionPos& pos);

    // ========================================================================
    // 区块光照初始化
    // ========================================================================

    /**
     * @brief 照亮区块
     *
     * 执行完整的光照计算，包括天空光照和方块光照传播。
     * 用于区块加载/生成时的初始光照计算。
     *
     * @param chunk 要照亮的目标区块
     * @param needsEdgeChecks 是否需要检查边缘（首次光照时为true）
     */
    void lightChunk(const IChunk* chunk, bool needsEdgeChecks = true);

    /**
     * @brief 照亮区块（使用指定 provider）
     *
     * 与上面的 lightChunk 等价，但使用调用方传入的 provider 而非 m_provider。
     * 用于 LIGHT 生成阶段在 worker 线程执行光照：传入 ChunkLightingProvider
     * （no-op markLightChanged，半径2邻居经 getChunkForLight fallback），
     * 避免触碰主线程独占状态（ServerWorld::markLightChanged 的网络回调等）。
     *
     * @param provider 光照提供者（区块/方块状态访问 + markLightChanged 回调）
     * @param chunk 要照亮的目标区块
     * @param needsEdgeChecks 是否需要检查边缘
     */
    void lightChunk(StarLightLightingProvider* provider, const IChunk* chunk, bool needsEdgeChecks);

    /**
     * @brief 更新方块光引擎的空区块段映射（线程安全，持 m_mutex）
     *
     * 对齐 LightSyncManager::initializeChunkLighting 调用 BlockStarLightEngine::updateEmptinessMap，
     * 但经 WorldLightManager 加锁，保证 LIGHT 阶段 worker 线程与主线程 tick/checkBlock 串行。
     * 仅方块光引擎有空映射（SkyStarLightEngine 无），故只调用 m_blockLight。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param chunk 区块数据（读取各区块段空状态）
     */
    void updateEmptinessMap(i32 chunkX, i32 chunkZ, const ChunkData* chunk);

    /**
     * @brief 强制加载区块光照数据
     *
     * 用于已正确光照的区块，只需要重新加载光照数据到缓存，
     * 然后检查边缘以确保与邻居区块的一致性。
     * 不执行完整的光照计算。
     *
     * @param chunk 区块
     * @param emptySections 空区块段标记数组
     */
    void forceLoadInChunk(const IChunk* chunk, const std::vector<bool>& emptySections);

    /**
     * @brief 检查区块边缘
     *
     * 当邻居区块加载时，检查并修复边缘光照不一致。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    void checkChunkEdges(i32 chunkX, i32 chunkZ);

    // ========================================================================
    // 调试信息
    // ========================================================================

    /**
     * @brief 获取调试信息
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @return 调试字符串
     */
    [[nodiscard]] std::string getDebugInfo(LightType type, const SectionPos& pos) const;

private:
    mutable std::recursive_mutex m_mutex;
    std::unique_ptr<BlockStarLightEngine> m_blockLight;
    std::unique_ptr<SkyStarLightEngine> m_skyLight;
    StarLightLightingProvider* m_provider;
    bool m_hasBlockLight;
    bool m_hasSkyLight;
};

} // namespace mc
