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

#include "Region.hpp"

// 前向声明
namespace mc {
class IWorld;
}

namespace mc::entity::ai::pathfinding {

/**
 * @brief 委托 IWorld 的 Region 实现
 *
 * Region 接口历史上是纯抽象类，但全仓无任何具体子类实现，m_region 恒 nullptr，
 * 致 WalkNodeProcessor::getNodeType 在 `!m_region` 处恒返回 Blocked、getStartNode
 * 返回 nullptr、PathFinder::findPath 返回空路径——所有 MobEntity 的主动寻路
 * （MeleeAttackGoal / FoxFollowTargetGoal / RandomWalkingGoal 等）全部失效。
 *
 * 本类把寻路所需的有限世界访问委托到 IWorld，作为寻路器与世界之间的适配层。
 * 对应 MC Java 的 PathNavigation 区域缓存（LevelChunk 采样）。
 *
 * 生命周期：WorldRegion 由 PathNavigator::moveTo 在每次寻路前栈上构造并传入
 * PathFinder::setRegion，寻路结束后随栈帧析构，无需持有。
 */
class WorldRegion : public Region {
public:
    explicit WorldRegion(const IWorld& world) noexcept
        : m_world(&world)
    {}

    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isLoaded(i32 x, i32 z) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] bool isWalkable(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isWater(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isLava(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool canSeeSky(i32 x, i32 y, i32 z) const override;

private:
    const IWorld* m_world;
};

} // namespace mc::entity::ai::pathfinding
