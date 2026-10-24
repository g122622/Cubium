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

#include "Ray.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

/**
 * @brief 射线检测上下文
 *
 * 包含射线检测所需的所有参数。
 * 参考MC的RayTraceContext。
 */
struct RaycastContext {
    Ray ray;
    f32 maxDistance = 5.0f; ///< MC默认5格，创造模式更远

    RaycastContext() = default;

    /**
     * @brief 构造射线检测上下文
     * @param r 射线
     * @param dist 最大检测距离
     */
    explicit RaycastContext(const Ray& r, f32 dist = 5.0f)
        : ray(r)
        , maxDistance(dist)
    {}

    /**
     * @brief 获取射线终点
     */
    [[nodiscard]] Vector3 endPosition() const
    {
        return ray.at(maxDistance);
    }
};

/**
 * @brief 执行方块射线检测
 *
 * 使用DDA算法沿射线遍历方块，并基于方块 shape 计算命中结果。
 *
 * 算法说明（参考MC的IBlockReader.doRayTrace）：
 * 1. 计算射线在各轴上穿过一个方块所需的时间比率
 * 2. 每次选择最近的方块边界进入
 * 3. 对候选方块的 shape 做精确包围盒相交测试
 * 4. 超出距离限制或Y范围时停止
 * 5. 区块未加载（nullptr）视为空气，继续检测
 *
 * @param context 射线参数
 * @param world 方块读取器
 * @return 检测结果，若未击中返回miss
 */
[[nodiscard]] BlockRaycastResult raycastBlocks(const RaycastContext& context, const IWorld& world);

/**
 * @brief 沿射线查找首个水源方块（对应 vanilla Player#getPlayerPOVHitResult(..., Fluid.SOURCE_ONLY)）
 *
 * 对每个步进到的方块先判定其流体状态：若为流动流体则立即终止（对应 ClipContext.Fluid.SOURCE_ONLY
 * 的流体行为——流动流体会挡住射线，只是不产生命中）；若为流体源，则命中该方块的流体碰撞形状
 * （高度按液体量缩放，液体量见 FluidState::getAmount）。射线终点与起点同 raycastBlocks 一样
 * 做端点微偏移，保证终点恰在方块内/边界时的判定稳定。
 *
 * 非流体方块的碰撞形状不参与本次检测（vanilla 中该调用只用于液体，调用方不会在同一次检测里
 * 同时关心实心方块）。
 *
 * @param context 射线参数
 * @param world 世界
 * @return 命中的流体源方块；未命中返回 miss
 */
[[nodiscard]] BlockRaycastResult raycastFluidSource(const RaycastContext& context, const IWorld& world);

} // namespace mc
