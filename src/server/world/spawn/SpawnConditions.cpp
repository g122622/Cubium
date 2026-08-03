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

#include "SpawnConditions.hpp"
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockState.hpp"

#include <algorithm>
#include <cmath>

namespace mc::world::spawn {

/**
 * @brief 光照等级检查
 *
 * 怪物: 光照等级 <= 7
 * 动物: 光照等级 > 7 (通常需要光照等级 7+)
 */
bool SpawnConditions::checkLightLevel(i32 skyLight, i32 blockLight, bool isMonster)
{
    // 计算有效光照等级
    i32 effectiveLight = std::max(skyLight, blockLight);

    if (isMonster) {
        // 怪物生成需要低光照
        return effectiveLight <= 7;
    } else {
        // 动物生成需要足够的光照
        return effectiveLight > 7;
    }
}

bool SpawnConditions::canSpawnAtPosition(IWorld& world, i32 x, i32 y, i32 z, f32 entityWidth, f32 entityHeight)
{
    // 边界检查
    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 检查碰撞空间
    if (!hasCollisionSpace(world, x, y, z, entityWidth, entityHeight)) {
        return false;
    }

    // 获取脚下方块
    const BlockState* belowBlock = world.getBlockState(x, y - 1, z);
    if (!belowBlock || belowBlock->isAir()) {
        return false;
    }

    // 检查方块是否阻止生成
    if (blockPreventsSpawn(belowBlock->isLiquid(), belowBlock->isAir())) {
        return false;
    }

    return true;
}

bool SpawnConditions::hasCollisionSpace(IWorld& world, i32 x, i32 y, i32 z, f32 width, f32 height)
{
    // 创建实体的碰撞箱
    // 实体中心在 x.5, y, z.5
    f32 halfWidth = width / 2.0f;
    f32 minX = static_cast<f32>(x) + 0.5f - halfWidth;
    f32 maxX = static_cast<f32>(x) + 0.5f + halfWidth;
    f32 minY = static_cast<f32>(y);
    f32 maxY = static_cast<f32>(y) + height;
    f32 minZ = static_cast<f32>(z) + 0.5f - halfWidth;
    f32 maxZ = static_cast<f32>(z) + 0.5f + halfWidth;

    // 边界检查
    if (minY < world::MIN_BUILD_HEIGHT || maxY > world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const i32 minBlockX = static_cast<i32>(std::floor(minX));
    const i32 minBlockY = static_cast<i32>(std::floor(minY));
    const i32 minBlockZ = static_cast<i32>(std::floor(minZ));
    const i32 maxBlockX = static_cast<i32>(std::floor(maxX));
    const i32 maxBlockY = static_cast<i32>(std::floor(maxY));
    const i32 maxBlockZ = static_cast<i32>(std::floor(maxZ));

    if (!world.isWithinWorldBounds(minBlockX, minBlockY, minBlockZ) ||
        !world.isWithinWorldBounds(maxBlockX, maxBlockY, maxBlockZ)) {
        return false;
    }

    // 使用 IWorld 的碰撞检测
    AxisAlignedBB box(minX, minY, minZ, maxX, maxY, maxZ);
    return !world.hasBlockCollision(box);
}

bool SpawnConditions::blockPreventsSpawn(bool isLiquid, bool isAir)
{
    // 液体方块阻止生成
    if (isLiquid) {
        return true;
    }

    // 空气方块阻止站立
    if (isAir) {
        return true;
    }

    return false;
}

i32 SpawnConditions::getGroundHeight(IWorld& world, i32 x, i32 z)
{
    // 从最高点向下搜索第一个可站立方块
    i32 height = world.getHeight(x, z);

    // 检查是否有有效的站立位置
    if (height > 0) {
        const BlockState* block = world.getBlockState(x, height - 1, z);
        if (block && !block->isAir() && !block->isLiquid()) {
            return height;
        }
    }

    return height;
}

bool SpawnConditions::isInWater(IWorld& world, i32 x, i32 y, i32 z)
{
    return world.isWaterAt(x, y, z);
}

bool SpawnConditions::isInLava(IWorld& world, i32 x, i32 y, i32 z)
{
    return world.isLavaAt(x, y, z);
}

} // namespace mc::world::spawn
