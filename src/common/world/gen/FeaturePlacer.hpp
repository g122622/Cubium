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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world::gen {

/**
 * @brief 按需放置特征的工具类
 *
 * 从已加载区块构建临时的 WorldGenRegion，使得 SaplingBlock.grow() 等游戏逻辑
 * 可以调用 TreeFeature::place() 等需要 WorldGenRegion 的方法。
 *
 * 对齐 MC Java 的 ServerLevel 直接实现 WorldGenLevel 接口的设计：
 * MC Java 中 SaplingBlock.advanceTree() 将 ServerLevel 传给 TreeGrower.growTree()，
 * 后者调用 ConfiguredFeature.place() 时直接使用 ServerLevel 作为 WorldGenLevel。
 * 在 Cubium 中，TreeFeature::place() 需要 WorldGenRegion&，
 * 因此本类负责在运行时从已加载区块构建 WorldGenRegion 桥接此差距。
 *
 * 使用方式：
 * @code
 * // 1. 收集区块（由调用方从 ServerChunkManager 获取）
 * std::vector<IChunk*> chunks;
 * ChunkCoord cx = world::toChunkCoord(pos.x);
 * ChunkCoord cz = world::toChunkCoord(pos.z);
 * // ... 收集 3x3 区块 ...
 *
 * // 2. 构建 WorldGenRegion
 * auto region = FeaturePlacer::createRegion(cx, cz, std::move(chunks), dimId);
 * if (region) {
 *     // 填充世界状态
 *     FeaturePlacer::populateWorldState(*region, seed, currentTick, dayTime, hardcore, difficulty);
 *     // 使用 region
 *     TreeFeature feature;
 *     feature.place(*region, rng, pos, config);
 * }
 * @endcode
 */
class FeaturePlacer {
public:
    /**
     * @brief 从已加载区块创建 WorldGenRegion
     *
     * 使用 FEATURES 步骤构建 WorldGenRegion（blockStateWriteRadius = 1）。
     *
     * @param centerChunkX 中心区块 X 坐标
     * @param centerChunkZ 中心区块 Z 坐标
     * @param chunks 已加载区块指针数组，按行优先排列
     *               大小为 (2*chunkRadius+1)^2，由调用方收集
     * @param chunkRadius 区块半径（1 = 3x3）
     * @param dimensionId 维度 ID
     * @return 创建的 WorldGenRegion
     */
    [[nodiscard]] static std::unique_ptr<WorldGenRegion> createRegion(ChunkCoord centerChunkX,
        ChunkCoord centerChunkZ,
        std::vector<IChunk*> chunks,
        i32 chunkRadius,
        DimensionId dimensionId = 0);

    /**
     * @brief 从世界属性填充 WorldGenRegion 的状态
     *
     * @param region 要填充的 WorldGenRegion
     * @param seed 世界种子
     * @param currentTick 当前 tick
     * @param dayTime 白天时间
     * @param hardcore 是否硬核模式
     * @param difficulty 难度
     */
    static void populateWorldState(
        WorldGenRegion& region, u64 seed, u64 currentTick, i64 dayTime, bool hardcore, Difficulty difficulty);
};

} // namespace world::gen
} // namespace mc
