/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/LightType.hpp"

namespace mc {

class WorldGenRegion;

namespace world::chunk {
class ChunkData;
}

namespace server {

class ServerWorld;

/**
 * @brief LIGHT 生成阶段的光照提供者适配器
 *
 * 在 LIGHT 状态步于 worker 线程执行光照时使用。与 ServerWorld（主线程 provider）的区别：
 *
 * 1. getChunkForLight：中心区块及半径1邻居经 WorldGenRegion 取 ChunkPrimer 底层 ChunkData
 *    （ChunkPrimer 未重写光照 nibble 接口，必须返回 ChunkData*）。半径2邻居（region 外）
 *    fallback 到 ServerWorld::getChunkForLight（从 m_chunks 取已发布 ChunkData*）。
 * 2. markLightChanged：no-op。LIGHT 阶段区块尚未进入 m_chunks（_storeChunkInMemorySync 在 FULL
 *    才调用），ServerWorld::markLightChanged 的 tryToGetChunkInMem 返回 nullptr；
 *    且 markLightChanged 会触发网络回调（m_onLightChanged）与 ChunkSection nibble 写入
 *    （_syncLightDataToChunk），均为主线程独占状态。light() 已通过 setNibbles 将结果写入
 *    ChunkData 的 SWMRNibbleArray，无需 markLightChanged 回写。
 *
 * 其余世界信息（hasSkyLight、建筑高度、section 数量、方块状态）委托 ServerWorld。
 */
class ChunkLightingProvider : public StarLightLightingProvider {
public:
    /**
     * @brief 构造光照提供者适配器
     *
     * @param world 服务端世界（半径2 fallback 与世界信息委托）
     * @param region 生成区域（中心+半径1邻居 ChunkPrimer 访问）
     * @param centerX 中心区块 X
     * @param centerZ 中心区块 Z
     */
    ChunkLightingProvider(ServerWorld& world, WorldGenRegion& region, ChunkCoord centerX, ChunkCoord centerZ);

    // === 区块访问 ===
    [[nodiscard]] IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override;
    [[nodiscard]] const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override;

    // === 方块状态 ===
    [[nodiscard]] const BlockState* getBlockStateForLight(const BlockPos& pos) const override;

    // === 世界信息 ===
    [[nodiscard]] IWorld* getWorld() override;
    [[nodiscard]] const IWorld* getWorld() const override;

    // === 光照通知（no-op，见类注释） ===
    void markLightChanged(LightType type, const SectionPos& pos) override;

    // === 维度信息 ===
    [[nodiscard]] bool hasSkyLight() const override;
    [[nodiscard]] i32 getMinBuildHeight() const override;
    [[nodiscard]] i32 getMaxBuildHeight() const override;
    [[nodiscard]] i32 getSectionCount() const override;

private:
    ServerWorld* m_world;
    WorldGenRegion* m_region;
    ChunkCoord m_centerX;
    ChunkCoord m_centerZ;
};

} // namespace server
} // namespace mc
