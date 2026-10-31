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

/**
 * @file WorldRegion.hpp
 * @brief 以 `IWorld` 为后端的 `Region`（对应原版 `PathNavigationRegion`）。
 *
 * 寻路只看世界的一小块方形区域。原版为此专门建了 `PathNavigationRegion`（按区块切片的
 * 快照）；本实现直接委托 `IWorld`，因为两者的可观测差异只有「未加载区域读什么」：
 * 原版对未加载区块返回空气与空流体，这里的 `IWorld` 对越界坐标同样返回空气。
 *
 * 生命周期：由 `PathNavigator` 在每次寻路 / 每次地面高度查询时于栈上构造，随栈帧析构；
 * 求值器只持有裸指针，会话结束时必须失效（见 `NodeEvaluator::done`）。
 */

#pragma once

#include "Region.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc::entity::ai::pathfinding {

/**
 * @brief 委托 `IWorld` 的区域视图
 */
class WorldRegion final : public Region {
public:
    explicit WorldRegion(const IWorld& world) noexcept
        : m_world(&world)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_world->getBlockState(x, y, z);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return m_world->getFluidState(x, y, z);
    }

    [[nodiscard]] bool hasCollision(const void* entity, const AxisAlignedBB& box) const override
    {
        // 原版的 noCollision(Entity, AABB) 只考虑方块碰撞形状，不含流体形状（水 / 细雪），
        // 因此这里也必须用方块碰撞查询，而不是带实体碰撞的完整物理查询。
        MC_UNUSED(entity);
        return m_world->hasBlockCollision(box);
    }

    [[nodiscard]] i32 minBuildHeight() const override
    {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] i32 maxBuildHeight() const override
    {
        return world::MAX_BUILD_HEIGHT;
    }

private:
    const IWorld* m_world;
};

} // namespace mc::entity::ai::pathfinding