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
 */

#pragma once

#include "common/world/lighting/IChunkLightProvider.hpp"
#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
class WorldLightManager;
namespace world::chunk {
class ChunkData;
}
using world::chunk::ChunkData;

namespace server {

class ServerWorld;

/**
 * @brief 运行时方块变更在 worker 线程传播用的光照提供者
 *
 * ③-1 阶段：运行时方块变更的光照传播从主线程同步搬到 worker 线程
 * （经 UniversalWorkerPool 区域互斥池 writeRadius=2 提交）。本 provider 是
 * worker 任务执行期间访问区块数据的适配层，与 LIGHT 生成阶段用的
 * ChunkLightingProvider 同构，但职责不同：
 *
 * 1. **区块保活**：构造时（主线程 drain 入队时）对中心区块 5×5 范围调
 *    ServerChunkManager::tryToGetChunkSharedInMem 拿 shared_ptr<ChunkData>
 *    副本存 m_keepaliveChunks（作用域保活）。worker 在途期间即使区块被
 *    主线程卸载（m_chunks erase），引用计数 >0 使 ChunkData 存活，避免 UAF。
 *    5×5 = writeRadius(2) 的两倍 + 1，与 setupCaches(loadTwoRadius=true)
 *    访问半径2邻居一致。m_chunkPtrs 存裸指针供引擎零开销索引。
 * 2. **markLightChanged 收集而非回调**：worker 侧不调主线程独占的
 *    ServerWorld::markLightChanged（其内部 _syncLightDataToChunk 写 ChunkSection
 *    nibble + m_onLightChanged 网络包，主线程独占）。改为 emplace 到
 *    m_dirtySections，任务完成后由 RuntimeLightTask 入主线程 flush 队列，
 *    主线程 tick 统一调真正的 markLightChanged。
 * 3. **维度信息/方块状态委托 ServerWorld**：getBlockStateForLight 在光照引擎
 *    传播期零调用（grep 确认 lighting 目录内不调），委托安全。
 *
 * 与 ChunkLightingProvider 的区别：ChunkLightingProvider 用于生成阶段
 * （区块是 ChunkPrimer，经 WorldGenRegion 取，markLightChanged no-op 因为
 * 区块尚未进 m_chunks）；RuntimeLightingProvider 用于运行时（区块已是
 * m_chunks 中的 ChunkData，markLightChanged 收集 dirty 待主线程 flush）。
 */
class RuntimeLightingProvider : public StarLightLightingProvider {
public:
    /**
     * @brief 构造运行时光照提供者
     *
     * 主线程调用：对中心 5×5 范围调 tryToGetChunkSharedInMem 保活。
     * 保活范围 = [centerX-2, centerX+2] × [centerZ-2, centerZ+2]。
     *
     * @param world 服务端世界（取 chunkManager 保活、维度信息委托）
     * @param centerX 中心区块 X
     * @param centerZ 中心区块 Z
     */
    RuntimeLightingProvider(ServerWorld& world, ChunkCoord centerX, ChunkCoord centerZ);

    // === 区块访问 ===
    [[nodiscard]] IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override;
    [[nodiscard]] const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override;

    // === 方块状态 ===
    [[nodiscard]] const BlockState* getBlockStateForLight(const BlockPos& pos) const override;

    // === 世界信息 ===
    [[nodiscard]] IWorld* getWorld() override;
    [[nodiscard]] const IWorld* getWorld() const override;

    // === 光照通知（收集 dirty section，见类注释） ===
    void markLightChanged(LightType type, const SectionPos& pos) override;

    // === 维度信息 ===
    [[nodiscard]] bool hasSkyLight() const override;
    [[nodiscard]] i32 getMinBuildHeight() const override;
    [[nodiscard]] i32 getMaxBuildHeight() const override;
    [[nodiscard]] i32 getSectionCount() const override;

    /**
     * @brief 取出 worker 传播期间收集的 dirty section 列表
     *
     * worker 任务执行完 checkBlocksWithProvider 后调用，取出 markLightChanged
     * 收集的全部 (LightType, SectionPos) 对，交给 RuntimeLightTask 入主线程
     * flush 队列。取出后内部列表清空。
     */
    [[nodiscard]] std::vector<std::pair<LightType, SectionPos>> takeDirtySections();

private:
    /// 5×5 区块保活范围（writeRadius=2 的两倍 + 1）
    static constexpr i32 KEEPALIVE_RADIUS = 2;
    static constexpr i32 KEEPALIVE_EXTENT = KEEPALIVE_RADIUS * 2 + 1;           // 5
    static constexpr i32 KEEPALIVE_COUNT = KEEPALIVE_EXTENT * KEEPALIVE_EXTENT; // 25

    /// (dx+radius) + (dz+radius)*extent → 5×5 平面索引
    [[nodiscard]] static constexpr i32 _keepaliveIndex(i32 dx, i32 dz) noexcept
    {
        return (dx + KEEPALIVE_RADIUS) + (dz + KEEPALIVE_RADIUS) * KEEPALIVE_EXTENT;
    }

    ServerWorld* m_world;
    ChunkCoord m_centerX;
    ChunkCoord m_centerZ;

    /// 5×5 shared_ptr 保活（worker 在途期间防止 ChunkData 被卸载释放）
    std::array<std::shared_ptr<ChunkData>, KEEPALIVE_COUNT> m_keepaliveChunks;

    /// 5×5 裸指针视图（指向 m_keepaliveChunks 内对象，引擎零开销索引）
    std::array<ChunkData*, KEEPALIVE_COUNT> m_chunkPtrs{};

    /// markLightChanged 收集的 dirty section（待主线程 flush）
    std::vector<std::pair<LightType, SectionPos>> m_dirtySections;
};

} // namespace server
} // namespace mc
