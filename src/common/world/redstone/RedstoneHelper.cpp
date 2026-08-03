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

#include "RedstoneHelper.hpp"

#include "../../entity/core/Entity.hpp"
#include "../../entity/inventory/IInventory.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../util/AxisAlignedBB.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <vector>

namespace mc {
namespace world {
namespace redstone {

bool RedstoneHelper::isNormalCube(const BlockState& state)
{
    // 实体方块：固体、不透明、非空气
    return state.isSolid() && state.isOpaque() && !state.isAir();
}

bool RedstoneHelper::canConnectRedstone(const BlockState& state)
{
    // 检查方块是否可以输出红石信号
    return state.getBlock().canProvidePower(state);
}

bool RedstoneHelper::isRedstoneConductor(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 红石导体检查：
    // 1. 必须是实体方块
    // 2. 不阻挡红石信号传输

    if (!isNormalCube(state)) {
        return false;
    }

    // 某些特殊方块虽然不是红石导体（如观察者、红石块等）
    // 这些需要单独检查

    // 默认情况下，实体方块都是红石导体
    return true;
}

i32 RedstoneHelper::getEntitySignal(IWorld& world, const BlockPos& pos)
{
    // 在指定方块位置构建 1x1x1 检测区域
    AxisAlignedBB searchBox = AxisAlignedBB::fromBlock(pos.x, pos.y, pos.z);
    return getEntitySignal(world, searchBox);
}

i32 RedstoneHelper::getEntitySignal(IWorld& world, const AxisAlignedBB& searchBox)
{
    // 获取区域内的所有实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

    // 遍历实体，查找最大的比较器信号值
    i32 maxSignal = 0;
    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }
        i32 signal = entity->getComparatorOutput();
        if (signal > maxSignal) {
            maxSignal = signal;
        }
    }

    return maxSignal;
}

i32 RedstoneHelper::calcRedstoneFromInventory(const IInventory& inventory)
{
    const i32 containerSize = inventory.getContainerSize();
    if (containerSize <= 0) {
        return 0;
    }

    f32 fillRatio = 0.0f;

    for (i32 i = 0; i < containerSize; ++i) {
        const ItemStack& stack = inventory.getItem(i);
        if (!stack.isEmpty()) {
            fillRatio += static_cast<f32>(stack.getCount()) / static_cast<f32>(stack.getMaxStackSize());
        }
    }

    fillRatio /= static_cast<f32>(containerSize);

    // lerpDiscrete(fillRatio, 0, 14) + (fillRatio > 0 ? 1 : 0)
    // 即 floor(fillRatio * 14) + (有物品 ? 1 : 0)
    i32 signal = static_cast<i32>(math::floorTo<i32>(fillRatio * 14.0f));
    if (fillRatio > 0.0f) {
        signal += 1;
    }

    return std::min(signal, MAX_POWER);
}

} // namespace redstone
} // namespace world
} // namespace mc
