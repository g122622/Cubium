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

#include "GroundPathNavigator.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

namespace {

/// 水面向上搜索的最大格数（原版 `getSurfaceY` 的 16 格上限）。
constexpr i32 MAX_SURFACE_SEARCH = 16;

} // namespace

GroundPathNavigator::GroundPathNavigator(MobEntity* mob, std::unique_ptr<PathFinder> pathFinder)
    : PathNavigator(mob, std::move(pathFinder))
{}

bool GroundPathNavigator::moveToBlock(const BlockPos& target, i32 reachRange, f64 speed)
{
    const BlockPos resolved = m_canPathToTargetsBelowSurface ? target : _findSurfacePosition(target);
    return moveTo(_createPath(resolved, reachRange, 16.0f), speed);
}

bool GroundPathNavigator::_canUpdatePath() const
{
    return m_mob->onGround() || m_mob->isInWater() || m_mob->hasPassengers();
}

Vector3d GroundPathNavigator::_getTempMobPosition() const
{
    return Vector3d(m_mob->x(), static_cast<f64>(_getSurfaceY()), m_mob->z());
}

i32 GroundPathNavigator::_getSurfaceY() const
{
    const IWorld* world = m_mob->world();
    MC_ASSERT_RELEASE(world != nullptr);
    if (!m_mob->isInWater() || !canFloat()) {
        return static_cast<i32>(std::floor(m_mob->y() + 0.5));
    }
    // 会浮水的实体在水里：向上找到水面，用于卡住判定与路径跟随。
    i32 y = static_cast<i32>(std::floor(m_mob->y()));
    const i32 blockX = static_cast<i32>(std::floor(m_mob->x()));
    const i32 blockZ = static_cast<i32>(std::floor(m_mob->z()));
    i32 steps = 0;
    while (world->isWaterAt(blockX, y, blockZ)) {
        ++y;
        if (++steps > MAX_SURFACE_SEARCH) {
            return static_cast<i32>(std::floor(m_mob->y()));
        }
    }
    return y;
}

BlockPos GroundPathNavigator::_findSurfacePosition(const BlockPos& target) const
{
    const IWorld* world = m_mob->world();
    MC_ASSERT_RELEASE(world != nullptr);

    if (world->getBlockState(target.x, target.y, target.z)->isAir()) {
        // 目标是空气：向下找到第一块非空气，取其上方一格。
        i32 y = target.y - 1;
        while (y >= world::MIN_BUILD_HEIGHT && world->getBlockState(target.x, y, target.z)->isAir()) {
            --y;
        }
        if (y >= world::MIN_BUILD_HEIGHT) {
            return BlockPos(target.x, y + 1, target.z);
        }
        // 一路到世界底部都是空气：改为向上找第一块非空气。
        y = target.y + 1;
        while (y <= world::MAX_BUILD_HEIGHT && world->getBlockState(target.x, y, target.z)->isAir()) {
            ++y;
        }
        return BlockPos(target.x, y, target.z);
    }

    if (!world->getBlockState(target.x, target.y, target.z)->isSolid()) {
        return target;
    }
    // 目标本身是实心方块：向上找到第一块空气，站在它上面。
    i32 y = target.y + 1;
    while (y <= world::MAX_BUILD_HEIGHT && world->getBlockState(target.x, y, target.z)->isSolid()) {
        ++y;
    }
    return BlockPos(target.x, y, target.z);
}

void GroundPathNavigator::_trimPath()
{
    PathNavigator::_trimPath();
    _trimPathSun();
}


} // namespace mc::entity::ai::pathfinding