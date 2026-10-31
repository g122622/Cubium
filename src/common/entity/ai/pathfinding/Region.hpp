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
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

class BlockState;
namespace fluid {
class FluidState;
}

/**
 * @brief 寻路可见的世界视图
 *
 * 寻路只通过这一个窄接口读世界：方块状态、流体状态、建造高度与碰撞查询。
 * 把「读世界」和「判定方块属于哪类节点」分开，是因为后者必须与实体无关
 * （同一格对任何实体都是同一个路径类型），只有前者随世界实现变化。
 *
 * 原版对应的就是 `CollisionGetter` 在 `PathNavigationRegion` 上的那张脸：
 * 只读、只覆盖以实体为中心的一块区域、外加 `noCollision(AABB)`。
 *
 * 生命周期：由 `PathNavigator` 在每次寻路时于栈上构造，寻路结束后随栈帧析构；
 * 求值器（`NodeEvaluator`）只持有它的裸指针，会话结束时必须置空。
 */
class Region {
public:
    virtual ~Region() = default;

    /**
     * @brief 读方块状态
     * @param x, y, z 方块坐标
     * @return 方块状态；超出建造高度时返回空气
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 读流体状态
     * @param x, y, z 方块坐标
     * @return 流体状态；超出建造高度时返回空流体
     */
    [[nodiscard]] virtual const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 给定盒子是否与实体发生了碰撞
     *
     * 原版 `CollisionGetter.noCollision(Entity, AABB)` 的反面：只考虑方块碰撞形状，
     * 不含流体形状（细雪与水的流体形状另算）。
     *
     * @param entity 参与碰撞判定的实体（可能为 nullptr，表示不带实体上下文）
     * @param box 世界坐标下的盒子
     * @return true 表示有碰撞
     */
    [[nodiscard]] virtual bool hasCollision(const void* entity, const AxisAlignedBB& box) const = 0;

    /**
     * @brief 世界最低建造高度（该 Y 之下不可放置方块）
     */
    [[nodiscard]] virtual i32 minBuildHeight() const = 0;

    /**
     * @brief 世界最高建造高度（该 Y 之上不可放置方块）
     */
    [[nodiscard]] virtual i32 maxBuildHeight() const = 0;
};

} // namespace mc