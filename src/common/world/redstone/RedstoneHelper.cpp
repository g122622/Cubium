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

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
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
    // 对齐原版语义：BlockBehaviour 的默认判据是「碰撞形状完整覆盖整格」
    // （isCollisionShapeFullBlock），随后由各方块注册时的 BlockProperties#isRedstoneConductor
    // 逐个覆盖；原版 Blocks.java 里共 22 处显式覆盖，绝大多数是 Blocks::never。
    //
    // 两个判据缺一不可，任意单独使用都会出错：
    // - 只看「isSolid && isOpaque」：单层台阶、楼梯碰撞形状不完整但材质不透明，会被误判为
    //   导体，于是「火把在台阶正下方」时凭空向四周输出满功率（原版此处为 0）。
    // - 只看「碰撞形状完整」：玻璃、红石块碰撞形状完整，但原版显式注册成 never，不是导体。
    //
    // TODO: 原版的显式 never 覆盖无法用「形状 + 不透明度」唯一表达（红石块是不透明且形状完整
    //   却被排除）。目前这组判据恰好与已覆盖的方块一致；后续若发现新的不一致，需要为相应方块
    //   引入与 BlockProperties#isRedstoneConductor 等价的逐方块覆盖，而不是继续堆砌特判。
    //   已知原版为 never 的完整清单见 Blocks.java 中 22 处 isRedstoneConductor(...) 调用。
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return state.getCollisionShape().coversFullBlock() && state.isOpaque();
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
