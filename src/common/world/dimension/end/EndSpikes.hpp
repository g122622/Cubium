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
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/WorldConstants.hpp"

#include <vector>

namespace mc {

/**
 * @brief 黑曜石柱状态
 *
 * 存储单根黑曜石柱的生成状态。
 * 字段对齐 MC 1.21.11 SpikeFeature.EndSpike。
 *
 * 本结构体从 server 侧 EndSpikeFeature 上移到 common 层，因为：
 * - EndDragonFight（common）需要 generateSpikes + getTopBoundingBox 查找柱顶末影水晶
 * - DragonRespawnAnimation（common）需要 generateSpikes 重建柱子
 * 而 EndSpike / generateSpikes 只依赖 common 层符号（math::Random、AxisAlignedBB 等），
 * 无 server gen 依赖，可安全上移，消除 common 对 server EndSpikeFeature 的反向依赖。
 */
struct EndSpike {
    i32 centerX;  ///< 中心X坐标（方块坐标）
    i32 centerZ;  ///< 中心Z坐标（方块坐标）
    i32 radius;   ///< 半径（2-5）
    i32 height;   ///< 高度（76-103）
    bool guarded; ///< 是否有铁栏杆笼子

    EndSpike(i32 x, i32 z, i32 r, i32 h, bool g)
        : centerX(x)
        , centerZ(z)
        , radius(r)
        , height(h)
        , guarded(g)
    {}

    /**
     * @brief 检查柱子中心是否在指定区块内
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 中心是否在该区块范围内
     */
    [[nodiscard]] bool isCenterWithinChunk(i32 chunkX, i32 chunkZ) const
    {
        return (centerX >> 4) == chunkX && (centerZ >> 4) == chunkZ;
    }

    /**
     * @brief 获取柱子顶部碰撞箱
     *
     * 用于在世界中查找位于柱顶的末影水晶实体（updateCrystalCount / resetSpikeCrystals）。
     * 碰撞箱为覆盖整个 Y 轴的柱形区域（半径 = spike.radius），与 MC 1.21.11
     * SpikeFeature.EndSpike.topBoundingBox 一致。
     *
     * @return 顶部碰撞箱（X/Z 覆盖柱子圆形外接方形，Y 覆盖整个世界高度）
     */
    [[nodiscard]] AxisAlignedBB getTopBoundingBox() const
    {
        return AxisAlignedBB(static_cast<f64>(centerX - radius),
            static_cast<f64>(world::MIN_BUILD_HEIGHT),
            static_cast<f64>(centerZ - radius),
            static_cast<f64>(centerX + radius),
            static_cast<f64>(world::MAX_BUILD_HEIGHT),
            static_cast<f64>(centerZ + radius));
    }
};

/**
 * @brief 生成默认的黑曜石柱配置（10根柱子）
 * @param worldSeed 世界种子
 * @return 黑曜石柱列表
 */
[[nodiscard]] std::vector<EndSpike> generateSpikes(u64 worldSeed);

} // namespace mc
